
#include "receiver.h"
#include "cleanup.h"

#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <stdbool.h>

#include "tune.h"
#include "tunebridge.h"

#define INVALID_FD -1
#define SEQ_TRACK_MAX 1024

static uint32_t written_seqnos[SEQ_TRACK_MAX];
static int seq_track_index = 0;

ssize_t recvall(int sock, void *buf, size_t len)
{
    size_t total = 0;
    char *p = buf;

    while (total < len) {
        ssize_t n = recv(sock, p + total, len - total, 0);
        if (n <= 0) {
            LOG(LOG_ERROR, "[recvall] recv() hatası: %s (fd=%d, total=%zu/%zu)", strerror(errno), sock, total, len);
            return -1;
        }

        LOG(LOG_DEBUG, "[recvall] %zd byte okundu (fd=%d, toplam: %zu/%zu)", n, sock, total + n, len);
        total += n;
    }

    LOG(LOG_INFO, "[recvall] Tüm veri başarıyla alındı (fd=%d, toplam: %zu byte)", sock, total);
    return total;
}


static bool seqno_already_written(uint32_t seq_no) {
    for (int i = 0; i < seq_track_index; ++i) {
        if (written_seqnos[i] == seq_no) return true;
    }
    return false;
}

static void track_written_seqno(uint32_t seq_no) {
    if (seq_track_index < SEQ_TRACK_MAX) {
        written_seqnos[seq_track_index++] = seq_no;
    } else {
        memmove(&written_seqnos[0], &written_seqnos[1], sizeof(uint32_t) * (SEQ_TRACK_MAX - 1));
        written_seqnos[SEQ_TRACK_MAX - 1] = seq_no;
    }
}

int receiver_main_loop(int tap_fd, int conn_fds[], int channel_count) {
    LOG(LOG_INFO, "Receiver başlatıldı (TCP). %d kanal üzerinden veri alınacak.", channel_count);

    fd_set master_fds, read_fds;
    FD_ZERO(&master_fds);
    FD_ZERO(&read_fds);

    int fd_max = tap_fd;
    for (int i = 0; i < channel_count; ++i) {
        if (conn_fds[i] != INVALID_FD) {
            FD_SET(conn_fds[i], &master_fds);
            if (conn_fds[i] > fd_max) fd_max = conn_fds[i];
        }
    }

    while (1) {
        read_fds = master_fds;

        int current_max_fd = 0;
        bool has_valid_fd = false;
        for (int i = 0; i < channel_count; ++i) {
            if (conn_fds[i] != INVALID_FD && FD_ISSET(conn_fds[i], &master_fds)) {
                if (conn_fds[i] > current_max_fd) current_max_fd = conn_fds[i];
                has_valid_fd = true;
            }
        }

        if (!has_valid_fd) {
            LOG(LOG_INFO, "Aktif bağlantı kalmadı. Receiver durduruluyor.");
            break;
        }

        int ret = select(current_max_fd + 1, &read_fds, NULL, NULL, NULL);
        if (ret < 0) {
            if (errno == EINTR) continue;
            LOG(LOG_ERROR, "select() hatası: %s", strerror(errno));
            return -1;
        }

        for (int i = 0; i < channel_count; ++i) {
            if (conn_fds[i] == INVALID_FD || !FD_ISSET(conn_fds[i], &read_fds)) continue;

            tap_packet_t packet;
            size_t header_size = offsetof(tap_packet_t, payload);
            if (recvall(conn_fds[i], &packet, header_size) <= 0) {
                close(conn_fds[i]);
                cleanup_unregister_socket(conn_fds[i]); //soketi kapattik, takibi bırakalim.
                FD_CLR(conn_fds[i], &master_fds);
                conn_fds[i] = INVALID_FD;
                continue;
            }

            if (packet.length == 0 || packet.length > MAX_PACKET_SIZE) {
                LOG(LOG_WARN, "Geçersiz paket uzunluğu: %u", packet.length);
                continue;
            }

            if (recvall(conn_fds[i], packet.payload, packet.length) <= 0) {
                LOG(LOG_WARN, "Payload alımı başarısız (kanal %d, seq=%u)", i, packet.seq_no);
                continue;
            }

            if ((packet.payload[0] & 0xF0) != 0x40) {
                LOG(LOG_WARN, "Geçersiz IP başlığı (offset 14, byte: 0x%02x) seq=%u, kanal=%d",
                    packet.payload[14], packet.seq_no, i);
                continue;
            }

            if (seqno_already_written(packet.seq_no)) {
                LOG(LOG_DEBUG, "seq_no=%u daha önce TUNE'e yazıldı, kanal %d atlandı", packet.seq_no, i);
                continue;
            }

            ssize_t w = tune_write(tap_fd, packet.payload, packet.length);
            if (w < 0) {
                LOG(LOG_ERROR, "TUNE'e yazma hatası (seq=%u, kanal %d): %s", packet.seq_no, i, strerror(errno));
            } else {
                LOG(LOG_INFO, "TUNE'e yazıldı ← kanal %d (seq=%u, %u byte)", i, packet.seq_no, packet.length);
                track_written_seqno(packet.seq_no);
            }
        }
    }

    LOG(LOG_INFO, "Receiver döngüsü sonlandı.");
    return 0;
}
