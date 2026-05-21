// duplex_multi.h
// Çok kanallı çift yönlü (full-duplex) TUN <-> TCP iletişimi

#ifndef DUPLEX_H
#define DUPLEX_H

// TUN'dan gelen verileri her kanala dağıtan ve kanallardan gelen verileri TUN'a yazan fonksiyon
int start_multi_duplex(int tun_fd,
                       const char **local_ips,
                       const char **remote_ips,
                       int *ports,
                       int channel_count,
                       int *sock_fds);  // yeni parametre eklendi


#endif // DUPLEX_H
