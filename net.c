#include "net.h"
#include "cleanup.h"  // cleanup entegrasyonu

#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <fcntl.h>
#include "tunebridge.h"
#include <stdbool.h>

#define NET_PORT 5000
#define NET_TIMEOUT_SEC 3

// Otomatik TCP bağlantısı kurar (önce server, olmazsa client)
int net_auto_connect(const char *remote_ip) {
    int listen_fd, conn_fd;
    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(NET_PORT),
        .sin_addr.s_addr = INADDR_ANY
    };

    // 1. Sunucu gibi başla
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_fd < 0) {
        LOG(LOG_ERROR, "[auto] socket (server) oluşturulamadı: %s", strerror(errno));
        goto attempt_client;
    }

    int opt = 1;
    setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) == 0) {
        listen(listen_fd, 1);
        LOG(LOG_INFO, "[auto] Dinleniyor... %d saniye", NET_TIMEOUT_SEC);

        fd_set set;
        struct timeval timeout = { .tv_sec = NET_TIMEOUT_SEC, .tv_usec = 0 };
        FD_ZERO(&set);
        FD_SET(listen_fd, &set);

        int rv = select(listen_fd + 1, &set, NULL, NULL, &timeout);
        if (rv > 0 && FD_ISSET(listen_fd, &set)) {
            conn_fd = accept(listen_fd, NULL, NULL);
            close(listen_fd);
            if (conn_fd >= 0) {
                LOG(LOG_INFO, "[auto] Bağlantı alındı (server)");
                cleanup_register_socket(conn_fd);
                return conn_fd;
            } else {
                LOG(LOG_ERROR, "[auto] accept hatası: %s", strerror(errno));
            }
        } else {
            LOG(LOG_WARN, "[auto] Dinleme süresi doldu, client modu deneniyor...");
        }

        close(listen_fd);
    } else {
        LOG(LOG_WARN, "[auto] bind başarısız: %s → client deneniyor", strerror(errno));
        close(listen_fd);
    }

attempt_client:
    // 2. Client olarak bağlanmayı dene
    int client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        LOG(LOG_ERROR, "[auto] socket (client) oluşturulamadı: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in remote = {
        .sin_family = AF_INET,
        .sin_port = htons(NET_PORT),
    };
    if (inet_pton(AF_INET, remote_ip, &remote.sin_addr) != 1) {
        LOG(LOG_ERROR, "[auto] Geçersiz IP adresi: %s", remote_ip);
        close(client_fd);
        return -1;
    }

    if (connect(client_fd, (struct sockaddr *)&remote, sizeof(remote)) < 0) {
        LOG(LOG_ERROR, "[auto] connect hatası (%s:%d): %s", remote_ip, NET_PORT, strerror(errno));
        close(client_fd);
        return -1;
    }

    LOG(LOG_INFO, "[auto] Client bağlantısı başarılı → %s:%d", remote_ip, NET_PORT);
    cleanup_register_socket(client_fd);
    return client_fd;
}

int net_auto_connect_with_role(const char *local_ip, const char *remote_ip, bool prefer_server) {
    if (prefer_server) {
        int fd = create_tcp_server_socket(local_ip, NET_PORT);
        if (fd >= 0) {
            LOG(LOG_INFO, "[auto] Server olarak dinleniyor (%s:%d)", local_ip, NET_PORT);
            int conn = accept_tcp_connection(fd);
            close(fd);
            return conn;
        }
    }

    // fallback to client
    int fd = create_tcp_client_socket(remote_ip, NET_PORT);
    if (fd >= 0) {
        LOG(LOG_INFO, "[auto] Client olarak bağlandı (%s:%d)", remote_ip, NET_PORT);
        return fd;
    }

    return -1;
}


// TCP client soketi (sender) için bağlantı kurulumu
int create_tcp_client_socket(const char *dst_ip, int dst_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG(LOG_ERROR, "TCP socket oluşturulamadı: %s", strerror(errno));
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(dst_port);
    if (inet_pton(AF_INET, dst_ip, &server_addr.sin_addr) != 1) {
        LOG(LOG_ERROR, "Geçersiz hedef IP adresi: %s", dst_ip);
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        LOG(LOG_ERROR, "TCP connect hatası (%s:%d): %s", dst_ip, dst_port, strerror(errno));
        close(sockfd);
        return -1;
    }

    LOG(LOG_INFO, "Bağlantı kuruldu → %s:%d", dst_ip, dst_port);

    cleanup_register_socket(sockfd);  // cleanup sistemine kaydet

    return sockfd;
}

// TCP server soketi (receiver) için dinleme başlatır
int create_tcp_server_socket(const char *bind_ip, int bind_port) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        LOG(LOG_ERROR, "TCP server socket oluşturulamadı: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(bind_port);
    if (inet_pton(AF_INET, bind_ip, &addr.sin_addr) != 1) {
        LOG(LOG_ERROR, "Geçersiz bind IP adresi: %s", bind_ip);
        close(sockfd);
        return -1;
    }

    if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        LOG(LOG_ERROR, "TCP bind hatası (%s:%d): %s", bind_ip, bind_port, strerror(errno));
        close(sockfd);
        return -1;
    }

    if (listen(sockfd, 1) < 0) {
        LOG(LOG_ERROR, "TCP listen hatası: %s", strerror(errno));
        close(sockfd);
        return -1;
    }

    LOG(LOG_INFO, "Dinleniyor ← %s:%d", bind_ip, bind_port);

    cleanup_register_socket(sockfd);  // cleanup sistemine kaydet

    return sockfd;
}

// TCP server için bağlantı kabul et
int accept_tcp_connection(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t len = sizeof(client_addr);
    int conn_fd = accept(server_fd, (struct sockaddr *)&client_addr, &len);
    if (conn_fd < 0) {
        LOG(LOG_ERROR, "TCP accept hatası: %s", strerror(errno));
        return -1;
    }

    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, ip_str, sizeof(ip_str));
    LOG(LOG_INFO, "Yeni bağlantı kabul edildi: %s:%d", ip_str, ntohs(client_addr.sin_port));

    return conn_fd;
}

// Socket'i arayüze sabitler
int bind_socket_on_interface(int sockfd, const char *ifname) {
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, ifname, strlen(ifname)) < 0) {
        LOG(LOG_WARN, "Socket arayüze bağlanamadı (%s): %s", ifname, strerror(errno));
        return -1;
    }
    return 0;
}
