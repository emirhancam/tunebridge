/*
 * cleanup.c
 *
 * Created on: May 20, 2025
 * Author: ecam
 */

#include "cleanup.h"
#include "log.h"

#include <stdlib.h>  // atexit, exit
#include <unistd.h>  // close, write, STDERR_FILENO
#include <signal.h>  // signal, sig_atomic_t, raise, SIG_DFL, sigaction, sigemptyset, memset
#include <stdbool.h> // bool
#include <errno.h>   // errno
#include <string.h>  // strerror, strlen, snprintf, strncat

#define MAX_TRACKED_SOCKETS 64
#define INVALID_FD -1 // Geçersiz dosya tanıtıcısı için bir işaretleyici

static int tracked_sockets[MAX_TRACKED_SOCKETS];
static int socket_count = 0;
static int tap_fd = INVALID_FD; // Başlangıçta geçersiz olarak ayarla

// Sinyal işleyicinin veya cleanup_run'ın birden fazla kez çalışmasını engellemek için bayrak
static volatile sig_atomic_t cleanup_in_progress = 0;

// TCP socket eklemek için
void cleanup_register_socket(int fd) {
    if (fd < 0) {
        // LOG(LOG_WARN, "Geçersiz socket fd (%d) kaydedilmeye çalışıldı.", fd);
        return;
    }

    for (int i = 0; i < socket_count; ++i) {
        if (tracked_sockets[i] == fd) {
            LOG(LOG_DEBUG, "Socket fd (%d) zaten temizlik için kayıtlı.", fd);
            return;
        }
    }

    if (socket_count < MAX_TRACKED_SOCKETS) {
        tracked_sockets[socket_count++] = fd;
        LOG(LOG_DEBUG, "Socket fd (%d) temizlik için kaydedildi.", fd);
    } else {
        LOG(LOG_WARN, "Maksimum socket takip sınırına ulaşıldı! fd (%d) kaydedilemedi.", fd);
    }
}

// Kayıtlı bir TCP soketinin kaydını silmek/işaretlemek için
void cleanup_unregister_socket(int fd) {
    if (fd < 0) {
        // LOG(LOG_WARN, "Geçersiz socket fd (%d) kaydı silinmeye çalışıldı.", fd);
        return;
    }
    bool found = false;
    for (int i = 0; i < socket_count; ++i) {
        if (tracked_sockets[i] == fd) {
            LOG(LOG_DEBUG, "Socket fd (%d) temizlik kaydından çıkarılıyor (INVALID_FD olarak işaretlendi).", fd);
            tracked_sockets[i] = INVALID_FD;
            found = true;
        }
    }
    if (!found) {
        LOG(LOG_WARN, "Socket fd (%d) temizlik kaydında bulunamadı (unregister).", fd);
    }
}


// TAP arayüzü eklemek için
void cleanup_register_tap(int fd) {
    if (fd < 0) {
        LOG(LOG_WARN, "Geçersiz TAP fd (%d) kaydedilmeye çalışıldı.", fd);
        return;
    }
    if (tap_fd != INVALID_FD && tap_fd != fd) {
        LOG(LOG_WARN, "Farklı bir TAP arayüzü (eski_fd=%d, yeni_fd=%d) zaten kayıtlı. Yenisiyle değiştiriliyor.", tap_fd, fd);
    } else if (tap_fd == fd) {
        LOG(LOG_DEBUG, "TAP fd (%d) zaten temizlik için kayıtlı.", fd);
        return;
    }
    tap_fd = fd;
    LOG(LOG_DEBUG, "TAP arayüzü fd (%d) temizlik için kaydedildi.", fd);
}

// Kayıtlı TAP arayüzünün kaydını silmek/işaretlemek için
void cleanup_unregister_tap(int fd) {
    if (fd < 0) {
        LOG(LOG_WARN, "Geçersiz TAP fd (%d) kaydı silinmeye çalışıldı.", fd);
        return;
    }
    if (tap_fd == fd) {
        LOG(LOG_DEBUG, "TAP fd (%d) temizlik kaydından çıkarılıyor (INVALID_FD olarak işaretlendi).", fd);
        tap_fd = INVALID_FD;
    } else if (tap_fd != INVALID_FD) {
        LOG(LOG_WARN, "TAP fd (%d) temizlik kaydında bulunamadı veya farklı bir fd kayıtlı (kayıtlı_fd=%d).", fd, tap_fd);
    }
}

// Temizlik işlemini yapan fonksiyon
void cleanup_run(void) {
    if (cleanup_in_progress) {
        return;
    }
    cleanup_in_progress = 1;

    LOG(LOG_INFO, "Temizlik rutinleri başlatıldı...");

    if (tap_fd != INVALID_FD) {
        LOG(LOG_DEBUG, "TAP arayüzü (fd=%d) kapatılıyor...", tap_fd);
        if (close(tap_fd) == 0) {
            LOG(LOG_INFO, "TAP arayüzü başarıyla kapatıldı (fd=%d).", tap_fd);
        } else {
            LOG(LOG_ERROR, "TAP arayüzü kapatılırken hata oluştu (fd=%d): %s", tap_fd, strerror(errno));
        }
        tap_fd = INVALID_FD;
    }

    LOG(LOG_DEBUG, "Kayıtlı %d socket girdisi temizleniyor...", socket_count);
    for (int i = 0; i < socket_count; ++i) {
        if (tracked_sockets[i] != INVALID_FD) {
            LOG(LOG_DEBUG, "Socket (fd=%d) kapatılıyor...", tracked_sockets[i]);
            if (close(tracked_sockets[i]) == 0) {
                LOG(LOG_INFO, "Socket başarıyla kapatıldı (fd=%d).", tracked_sockets[i]);
            } else {
                LOG(LOG_ERROR, "Socket kapatılırken hata oluştu (fd=%d): %s", tracked_sockets[i], strerror(errno));
            }
            tracked_sockets[i] = INVALID_FD;
        }
    }
    socket_count = 0;

    LOG(LOG_INFO, "Temizlik rutinleri tamamlandı.");
}

static void handle_signal(int sig) {
    const char* signal_name = "Bilinmeyen sinyal";
    switch(sig) {
        case SIGINT: signal_name = "SIGINT"; break;
        case SIGTERM: signal_name = "SIGTERM"; break;
        case SIGQUIT: signal_name = "SIGQUIT"; break;
    }

    // İsteğiniz üzerine LOG makrosu kullanılıyor.
    // Sinyal güvenliği konusunda dikkatli olunmalıdır.
    LOG(LOG_WARN, "%s (%d) yakalandı. Kaynaklar serbest bırakılıyor...", signal_name, sig);

    if (!cleanup_in_progress) {
        cleanup_run();
    }

    struct sigaction sa_dfl;
    memset(&sa_dfl, 0, sizeof(sa_dfl));
    sa_dfl.sa_handler = SIG_DFL;
    sigaction(sig, &sa_dfl, NULL);
    raise(sig);
}

void cleanup_init(void) {
    for (int i = 0; i < MAX_TRACKED_SOCKETS; ++i) {
        tracked_sockets[i] = INVALID_FD;
    }
    tap_fd = INVALID_FD;
    socket_count = 0;
    cleanup_in_progress = 0;

    if (atexit(cleanup_run) != 0) {
        // LOG(LOG_FATAL, ...) burada kullanılamaz çünkü log sistemi henüz hazır olmayabilir
        // veya atexit loglaması da sorun yaratabilir.
        fprintf(stderr, "FATAL: atexit ile cleanup_run kaydedilemedi!\n");
        // exit(EXIT_FAILURE); // Gerekirse burada programı sonlandır.
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_signal;
    sigemptyset(&sa.sa_mask);
    // sa.sa_flags = SA_RESTART;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction(SIGINT) hatası");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction(SIGTERM) hatası");
    }
    if (sigaction(SIGQUIT, &sa, NULL) == -1) {
        perror("sigaction(SIGQUIT) hatası");
    }
}
