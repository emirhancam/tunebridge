/*
 * monitor.c
 *
 *  Created on: May 20, 2025
 *      Author: ecam
 */


#include "monitor.h"
#include "log.h"
#include <sys/socket.h>
#include <errno.h>

volatile sig_atomic_t stop_flags[CHANNEL_COUNT] = {0};  // Her kanal için global flag

int is_channel_alive(int sockfd) {
    return send(sockfd, NULL, 0, MSG_NOSIGNAL) == 0;
}

void monitor_check_channels(const int *channel_fds, int channel_count) {
    for (int i = 0; i < channel_count; ++i) {
        if (!is_channel_alive(channel_fds[i])) {
            if (!stop_flags[i]) {
                LOG(LOG_WARN, "Kanal %d bağlantısı KAYIP! Durdurma başlatılıyor...", i);
                stop_flags[i] = 1;
            }
        } else {
            LOG(LOG_DEBUG, "Kanal %d bağlantısı AKTİF.", i);
        }
    }
}
