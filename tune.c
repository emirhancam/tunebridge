/*
 * tap.c
 *
 *  Created on: May 15, 2025
 *      Author: ecam
 */

#include "tune.h"

#include "cleanup.h"

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include "tunebridge.h"

// TUNE0 interface açılır ve file descriptor doner.
int tune_open(const char *ifname) {
    struct ifreq ifr;
    int fd;

    // TAP device açmak için /dev/net/tun açılır
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        LOG(LOG_ERROR, "/dev/net/tun açılamadı: %s", strerror(errno));
        return -1;
    }

    memset(&ifr, 0, sizeof(ifr));
    ifr.ifr_flags = IFF_TUN | IFF_NO_PI;  // TAP ve PI (proto info) olmadan

    //strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", ifname); //OPTIMIZASYON

    // TUNE interface'i ioctl ile sisteme tanıt
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        LOG(LOG_ERROR, "ioctl TUNSETIFF başarısız: %s", strerror(errno));
        close(fd);
        return -1;
    }

    LOG(LOG_INFO, "TUNE arayüzü '%s' başarıyla açıldı (fd = %d)", ifr.ifr_name, fd);
    cleanup_register_tap(fd);
    return fd;
}

// TAP'ten veri okuma
ssize_t tune_read(int fd, void *buf, size_t len) {
    ssize_t n = read(fd, buf, len);
    LOG(LOG_INFO, "TUNE'ten %zd byte okundu", n);
    if (n < 0) {
        LOG(LOG_ERROR, "Okuma hatası: %s", strerror(errno));
    }
    return n;
}

// TAP'e veri yazma
ssize_t tune_write(int fd, const void *buf, size_t len) {
	/*buf[0] = 0x00;
	buf[1] = 0x0c;
	buf[2] = 0x29;
	buf[3] = 0x3d;
	buf[4] = 0x43;
	buf[5] = 0xaa;
	buf[6] = 0x00;
	buf[7] = 0x0c;
	buf[8] = 0x29;
	buf[9] = 0x1f;
	buf[10] = 0xb3;
	buf[11] = 0xe8;*/

    ssize_t n = write(fd, buf, len);
    LOG(LOG_INFO, "TUNE'e %zd byte yazildi", n);
    if (n < 0) {
        LOG(LOG_ERROR, "Yazma hatası: %s", strerror(errno));
    }
    return n;
}

