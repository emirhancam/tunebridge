#include "duplex.h"
#include "tunebridge.h"
#include "log.h"
#include "net.h"
#include "cleanup.h"
#include "tune.h"
#include "sender.h"
#include "receiver.h"
#include "monitor.h" // stop_flags[]

#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <time.h>

#define BUF_SIZE 2000

typedef struct {
    int channel_id;
    int tun_fd;
    int sock_fd;
    uint32_t seq_no;
} duplex_arg_t;

// Header yapısı eklendi
typedef struct {
    uint8_t  channel_id;
    uint32_t seq_no;
    uint64_t timestamp_us;
    uint32_t length;
} __attribute__((packed)) tap_header_t;

static void* monitor_thread_fn(void* arg) {
    int *fds = (int*)arg;

    while (1) {
        monitor_check_channels(fds, CHANNEL_COUNT);

        int stopped = 1;
        for (int i = 0; i < CHANNEL_COUNT; ++i) {
            if (!stop_flags[i]) {
                stopped = 0;
                break;
            }
        }

        if (stopped) break;
        sleep(1);
    }

    LOG(LOG_DEBUG, "Monitor thread sonlandırılıyor, kaynaklar serbest bırakılıyor.");
    free(fds);  // Artık ulaşılıyor
    return NULL;
}



static void *tun_to_channel(void *arg) {
    duplex_arg_t *ctx = (duplex_arg_t *)arg;
    uint8_t payload[MAX_PACKET_SIZE];
    ssize_t n;

    while (!stop_flags[ctx->channel_id]) {
        n = tune_read(ctx->tun_fd, payload, MAX_PACKET_SIZE);
        if (n <= 0) {
            LOG(LOG_ERROR, "[CH%d] TUN okuma hatası: %s", ctx->channel_id, strerror(errno));
            stop_flags[ctx->channel_id] = 1;
            break;
        }

        tap_header_t hdr;
        hdr.channel_id = ctx->channel_id;
        hdr.seq_no = ctx->seq_no++;

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        hdr.timestamp_us = ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000;
        hdr.length = n;

        if (sendall(ctx->sock_fd, &hdr, sizeof(hdr)) <= 0) {
            LOG(LOG_ERROR, "[CH%d] Header gönderimi başarısız", ctx->channel_id);
            stop_flags[ctx->channel_id] = 1;
            break;
        }

        if (sendall(ctx->sock_fd, payload, n) <= 0) {
            LOG(LOG_ERROR, "[CH%d] Payload gönderimi başarısız", ctx->channel_id);
            stop_flags[ctx->channel_id] = 1;
            break;
        }

        LOG(LOG_INFO, "[CH%d] Sent: %zd byte (payload)", ctx->channel_id, n);
    }
    free(ctx); //new
    return NULL;
}


static void *channel_to_tun(void *arg) {
    duplex_arg_t *ctx = (duplex_arg_t *)arg;
    tap_header_t hdr;
    uint8_t payload[MAX_PACKET_SIZE];
    ssize_t n;

    while (!stop_flags[ctx->channel_id]) {
        n = recvall(ctx->sock_fd, &hdr, sizeof(hdr));
        if (n <= 0) {
            LOG(LOG_ERROR, "[CH%d] Header alımı başarısız", ctx->channel_id);
            stop_flags[ctx->channel_id] = 1;
            break;
        }

        if (hdr.length > 0 && hdr.length <= MAX_PACKET_SIZE) {
            n = recvall(ctx->sock_fd, payload, hdr.length);
            if (n <= 0) {
                LOG(LOG_ERROR, "[CH%d] Payload alımı başarısız", ctx->channel_id);
                stop_flags[ctx->channel_id] = 1;
                break;
            }

            tune_write(ctx->tun_fd, payload, hdr.length);
            LOG(LOG_DEBUG, "[CH%d] Received: %zd byte yazıldı", ctx->channel_id, hdr.length);
        } else {
            LOG(LOG_WARN, "[CH%d] Geçersiz paket uzunluğu: %u", ctx->channel_id, hdr.length);
        }
    }
    free(ctx); //new
    return NULL;
}


int start_multi_duplex(int tun_fd,
                       const char **local_ips,
                       const char **remote_ips,
                       int *ports,
                       int channel_count,
                       int *sock_fds) {

    pthread_t threads[channel_count * 2];
    /*duplex_arg_t *args = calloc(channel_count, sizeof(duplex_arg_t)); //AYRI AYRI iki CTX yapilinca gerek kalmadi..
    if (!args) {
        LOG(LOG_FATAL, "Bellek ayrılamadı");
        return -1;
    }*/

    for (int i = 0; i < channel_count; ++i) {
    	duplex_arg_t *ctx_send = malloc(sizeof(duplex_arg_t));
    	if (!ctx_send) {
    	    LOG(LOG_FATAL, "ctx_send için bellek ayrılamadı");
    	    return -1;
    	}

    	duplex_arg_t *ctx_recv = malloc(sizeof(duplex_arg_t));
    	if (!ctx_recv) {
    	    LOG(LOG_FATAL, "ctx_recv için bellek ayrılamadı");
    	    free(ctx_send); // Başarısızlık durumunda daha önce ayrılan ctx_send'i serbest bırak
    	    return -1;
    	}

        if (!ctx_send || !ctx_recv) {
            LOG(LOG_FATAL, "duplex_arg_t için bellek ayrılamadı");
            return -1;
        }

        ctx_send->channel_id = i;
        ctx_send->tun_fd = tun_fd;
        ctx_send->sock_fd = sock_fds[i];
        ctx_send->seq_no = 0;

        // ctx_recv'ye aynı değerleri kopyala. Ayni ctx free edilmesin diye iki ayri ctx yapildi!
        *ctx_recv = *ctx_send;

        pthread_create(&threads[i], NULL, tun_to_channel, ctx_send);
        pthread_create(&threads[i + channel_count], NULL, channel_to_tun, ctx_recv);
    }



    for (int i = 0; i < channel_count * 2; ++i) {
        pthread_detach(threads[i]);
    }

    // BURAYA MONITOR THREAD EKLENİYOR
    pthread_t monitor_thread;
    int *fds_copy = malloc(sizeof(int) * channel_count);
    if (!fds_copy) {
        LOG(LOG_FATAL, "Monitor için bellek ayrılamadı");
        return -1;
    }
    memcpy(fds_copy, sock_fds, sizeof(int) * channel_count);
    pthread_create(&monitor_thread, NULL, monitor_thread_fn, fds_copy);
    pthread_detach(monitor_thread);

    LOG(LOG_INFO, "Çok kanallı çift yönlü mod başlatıldı (%d kanal)", channel_count);
    return 0;
}
