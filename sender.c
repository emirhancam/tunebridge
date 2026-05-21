
/*
 * sender.c
 * Description:
 *              TAP'ten veri okunur, TCP ile bağlantılara gönderilir.
 *              Tüm kanal bağlantılarına aynı paket gönderilir.
 *              Paket, header ve payload olarak ayrılmıştır.
 */

#include "sender.h"
#include "monitor.h"
#include "cleanup.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>

#include "tune.h"
#include "tunebridge.h"

#define INVALID_FD -1
#define HEALTHCHECK_INTERVAL 5
static uint32_t seq_counters[CHANNEL_COUNT] = {0};

ssize_t sendall(int sock, const void *buf, size_t len) {
    size_t total = 0;
    const char *p = buf;
    while (total < len) {
        ssize_t n = send(sock, p + total, len - total, 0);
        if (n <= 0) return -1;
        total += n;
    }
    return total;
}

int sender_main_loop(int tap_fd,
                     const char *dst_ips[],
                     const int dst_ports[],
                     int sock_fds[],
                     int channel_count)
{
    time_t last_check = 0;
    cleanup_register_tap(tap_fd);
    for (int i = 0; i < channel_count; i++) {
        cleanup_register_socket(sock_fds[i]);
    }

    LOG(LOG_INFO, "Sender başlatıldı (TCP). TAP'ten veri okunacak ve %d bağlantıya iletilecek.", channel_count);

    while (1) {
        tap_packet_t packet;
        ssize_t n = tune_read(tap_fd, packet.payload, sizeof(packet.payload));
        if (n <= 0) {
            LOG(LOG_ERROR, "TUNE'ten veri okunamadı: %s", strerror(errno));
            break; //donguden cik.
        }

        packet.length = n;
        packet.timestamp_us = get_timestamp_us();

        for (int i = 0; i < channel_count; i++) {
            if (sock_fds[i] < 0) continue;

            packet.channel_id = (uint8_t)i;
            packet.seq_no = seq_counters[i]++;

            // Önce header'ı gönder
            size_t header_size = offsetof(tap_packet_t, payload);
            ssize_t h = sendall(sock_fds[i], &packet, header_size);
            if (h < 0) {
                LOG(LOG_ERROR, "Kanal %d → header gönderim hatası: %s", i, strerror(errno));
                close(sock_fds[i]);
                cleanup_unregister_socket(sock_fds[i]); //soketi kapattik artik takip etmeyelim.
                sock_fds[i] = INVALID_FD;
                continue;
            }

            // Ardından payload'ı gönder
            ssize_t p = sendall(sock_fds[i], packet.payload, packet.length);
            if (p < 0) {
                LOG(LOG_ERROR, "Kanal %d → payload gönderim hatası: %s", i, strerror(errno));
                close(sock_fds[i]);
                cleanup_unregister_socket(sock_fds[i]); //soketi kapattik artik takip etmeyelim.
                sock_fds[i] = INVALID_FD;
                continue;
            }

            LOG(LOG_INFO, "Kanal %d → Gönderildi: %zu byte (seq=%u)", i, h + p, packet.seq_no);
        }

        time_t now = time(NULL);
        if (now - last_check >= HEALTHCHECK_INTERVAL) {
            last_check = now;
            monitor_check_channels(sock_fds, channel_count);
        }
    }

    LOG(LOG_INFO, "Sender sonlandırılıyor, temizlik başlatılıyor...");
    //cleanup_run(); //Gereksiz olmus. atexit veya signal_handler tarafindan zaten cagrilacak.
    return 1;
}
