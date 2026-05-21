/**
 * @file sender.h
 * @brief TAP arayüzünden okunan verileri TCP üzerinden gönderen sender modülünün arayüzü.
 *
 * Bu modül, tapbridge projesinin gönderici (sender) tarafındaki ana işlevleri
 * ve yardımcı fonksiyonları içerir.
 *
 * @author Emirhan Çam
 * @date May 15, 2025
 */

#ifndef SENDER_H
#define SENDER_H

#include <stddef.h>		//size_t için
#include <sys/types.h>	//ssize_t için

/**
 * @brief Sender ana döngüsünü başlatır.
 *
 * TAP arayüzünden okunan Ethernet çerçevelerini paketleyip, belirtilen
 * TCP kanalları üzerinden alıcıya gönderir. Her paket tüm aktif kanallardan
 * gönderilir. Hata durumunda veya TAP arayüzünden okuma başarısız olduğunda
 * döngü sonlanabilir.
 *
 * @param[in] tap_fd        TAP arayüzünün dosya tanıtıcısı.
 * @param[in] dst_ips       Hedef (alıcı) IP adreslerini içeren string dizisi.
 * Her kanal için bir IP adresi (örn. {"10.1.1.2", "10.1.2.2", "10.1.3.2"}).
 * @param[in] dst_ports     Hedef (alıcı) port numaralarını içeren integer dizisi.
 * Her kanal için bir port numarası (örn. {5000, 5001, 5002}).
 * @param[in,out] sock_fds  Daha önce başarıyla açılmış ve bağlanmış TCP istemci soketlerini
 * içeren integer dizisi. Fonksiyon içinde, bir kanalda gönderim hatası
 * olursa ilgili soket kapatılıp dizide INVALID_FD (-1) olarak
 * işaretlenir.
 * @param[in] channel_count Kullanılacak kanal sayısı (dst_ips, dst_ports ve sock_fds dizilerinin
 * beklenen boyutu).
 * @return Döngü bir hata veya normal sonlanma nedeniyle bittiğinde genellikle 1 döner.
 * (Mevcut implementasyonda döngü sonlandığında 1 dönmektedir).
 */
int sender_main_loop(int tap_fd,
                     const char *dst_ips[],
                     const int dst_ports[],
                     /*const*/ int sock_fds[],
                     int channel_count);

/**
 * @brief Belirtilen soket üzerinden tüm veriyi göndermeyi garanti eder (blocking).
 *
 * TCP'nin akış tabanlı doğası gereği send() sistem çağrısı istenen tüm veriyi
 * tek seferde göndermeyebilir. Bu fonksiyon, `len` ile belirtilen tüm verinin
 * gönderildiğinden emin olana kadar send() çağrısını tekrarlar.
 *
 * @param[in] sock Gönderim yapılacak soket dosya tanıtıcısı.
 * @param[in] buf  Gönderilecek veriyi içeren buffer (bellek alanı).
 * @param[in] len  Gönderilecek verinin bayt cinsinden uzunluğu.
 * @return Başarılı olursa gönderilen toplam bayt sayısını (bu `len` değerine eşit olacaktır),
 * bir hata oluşursa (örneğin bağlantı kopması) -1 döner.
 */
ssize_t sendall(int sock, const void *buf, size_t len);

#endif // SENDER_H

