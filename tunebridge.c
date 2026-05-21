/*
 ==============================================================================
 Name        : TapBridge.c
 Author      : Emirhan Cam
 Version     : Temel Atma Toreni
 Copyright   :
 Description : Ana Kod
 ==============================================================================
 */

#include "tunebridge.h"

#include "sender.h"
#include "receiver.h"
#include "net.h"
#include "log.h"
#include "cleanup.h"
#include "tune.h"
#include "duplex.h"
#include "config.h"
#include "monitor.h"

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>

#define INVALID_FD -1
#define MAX_RETRY 5
#define RETRY_WAIT_SEC 1

bool is_ip_smaller(const char *ip1, const char *ip2) {
    struct in_addr a1, a2;
    inet_pton(AF_INET, ip1, &a1);
    inet_pton(AF_INET, ip2, &a2);
    return ntohl(a1.s_addr) < ntohl(a2.s_addr);
}

int main(int argc, char *argv[]) {
    cleanup_init();

    if (argc < 3 || strcmp(argv[1], "--duplex") != 0) {
        fprintf(stderr, "Kullanım: %s --duplex <config_file>\n", argv[0]);
        return 1;
    }

    config_t cfg;
    if (read_config(argv[2], &cfg) < 0) {
        LOG(LOG_FATAL, "Yapılandırma dosyası okunamadı: %s", argv[2]);
        return 1;
    }

    log_init("tunebridge", "./", LOG_DEBUG, LOG_FORMAT_TABLE);

    int channel_ports[CHANNEL_COUNT] = {BASE_PORT, BASE_PORT + 1, BASE_PORT + 2};

    if (cfg.role == 998) {
        int tun_fd = tune_open("tun0");
        if (tun_fd < 0) {
            LOG(LOG_FATAL, "TUN arayüzü açılamadı");
            return 1;
        }
        cleanup_register_tap(tun_fd);

        int sock_fds[CHANNEL_COUNT];
        for (int i = 0; i < CHANNEL_COUNT; ++i) {
            sock_fds[i] = INVALID_FD;
            bool prefer_server = is_ip_smaller(cfg.local_ips[i], cfg.remote_ips[i]);

            for (int retry = 0; retry < MAX_RETRY; ++retry) {
                sock_fds[i] = net_auto_connect_with_role(cfg.local_ips[i], cfg.remote_ips[i], prefer_server);
                if (sock_fds[i] >= 0)
                {
                    cleanup_register_socket(sock_fds[i]);  // cleanup modulune kayit.
                    break;  // retry döngüsünden çık
                }
                LOG(LOG_WARN, "[CH%d] Bağlantı kurulamadı, %d. tekrar deneniyor...", i, retry + 1);
                sleep(RETRY_WAIT_SEC);
            }

            if (sock_fds[i] < 0) {
                LOG(LOG_FATAL, "[CH%d] Tüm bağlantı denemeleri başarısız.", i);
                return 1;
            }
        }

        if (start_multi_duplex(tun_fd, cfg.local_ips, cfg.remote_ips, channel_ports, CHANNEL_COUNT, sock_fds) != 0) {
            LOG(LOG_FATAL, "Çoklu çift yönlü mod başlatılamadı");
            return 1;
        }

        LOG(LOG_INFO, "Duplex mod çalışıyor. Sonsuza kadar bekleniyor...");
        //while (1) pause();
        while (1) {
            int all_stopped = 1;
            for (int i = 0; i < CHANNEL_COUNT; ++i) {
                if (!stop_flags[i]) {
                    all_stopped = 0;
                    break;
                }
            }

            if (all_stopped) {
                LOG(LOG_WARN, "Tüm kanallar durduruldu. Sistem kapatılıyor.");
                cleanup_run();  // güvenli kapanış
                break;
            }

            sleep(1);  // kontrol döngüsü
        }
        return 0;
    }

    int tap_fd = tune_open("tun0");
    if (tap_fd < 0) {
        LOG(LOG_FATAL, "TUNE arayüzü açılamadı. Program sonlandırılıyor.");
        return 1;
    }
    cleanup_register_tap(tap_fd);

    int sock_fds[CHANNEL_COUNT];
    for(int i=0; i<CHANNEL_COUNT; ++i) sock_fds[i] = INVALID_FD;

    if (cfg.role == MODE_SENDER) {
        LOG(LOG_INFO, "Sender modu başlatılıyor...");
        for (int i = 0; i < CHANNEL_COUNT; i++) {
            sock_fds[i] = create_tcp_client_socket(cfg.remote_ips[i], channel_ports[i]);
            if (sock_fds[i] < 0) {
                LOG(LOG_FATAL, "Kanal %d için TCP istemci soketi oluşturulamadı (%s:%d). Program sonlandırılıyor.", i, cfg.remote_ips[i], channel_ports[i]);
                return 1;
            }
            cleanup_register_socket(sock_fds[i]);
        }
        sender_main_loop(tap_fd, cfg.remote_ips, channel_ports, sock_fds, CHANNEL_COUNT);

    } else if (cfg.role == MODE_RECEIVER) {
        LOG(LOG_INFO, "Receiver modu başlatılıyor...");
        int server_fds[CHANNEL_COUNT];
        for(int i=0; i<CHANNEL_COUNT; ++i) server_fds[i] = INVALID_FD;

        for (int i = 0; i < CHANNEL_COUNT; i++) {
            server_fds[i] = create_tcp_server_socket(cfg.local_ips[i], channel_ports[i]);
            if (server_fds[i] < 0) {
                LOG(LOG_FATAL, "Kanal %d için TCP sunucu dinleme soketi oluşturulamadı (%s:%d). Program sonlandırılıyor.", i, cfg.local_ips[i], channel_ports[i]);
                return 1;
            }
            cleanup_register_socket(server_fds[i]);
        }

        LOG(LOG_INFO, "Tüm kanallar için dinleme soketleri oluşturuldu. Bağlantılar bekleniyor...");

        for (int i = 0; i < CHANNEL_COUNT; i++) {
            sock_fds[i] = accept_tcp_connection(server_fds[i]);
            if (sock_fds[i] < 0) {
                LOG(LOG_FATAL, "Kanal %d için bağlantı kabul edilemedi. Program sonlandırılıyor.", i);
                return 1;
            }
            cleanup_register_socket(sock_fds[i]);

            if (server_fds[i] != INVALID_FD) {
                LOG(LOG_DEBUG, "Dinleme soketi (fd=%d) kapatılıyor ve kaydı siliniyor.", server_fds[i]);
                close(server_fds[i]);
                cleanup_unregister_socket(server_fds[i]);
                server_fds[i] = INVALID_FD;
            }
        }
        LOG(LOG_INFO, "Tüm kanallar için bağlantılar kabul edildi.");
        receiver_main_loop(tap_fd, sock_fds, CHANNEL_COUNT);
    }

    LOG(LOG_INFO, "Program normal şekilde sonlanıyor. Kaynaklar cleanup_run tarafından temizlenecek.");
    return 0;
}
