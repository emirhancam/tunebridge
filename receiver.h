/**
 * @file receiver.h
 * @brief TCP üzerinden gelen verileri alıp TAP arayüzüne yazan receiver modülünün arayüzü.
 *
 * Bu modül, `tapbridge` projesinin alıcı (receiver) tarafındaki ana işlevleri
 * ve yardımcı fonksiyonları içerir. Gelen paketlerin kopyalarını engellemek için
 * sıra numaralarını takip eder.
 *
 * @author Emirhan Çam
 * @date May 15, 2025
 */

#ifndef RECEIVER_H
#define RECEIVER_H

#include <stddef.h>     // size_t için
#include <sys/types.h>  // ssize_t için

/**
 * @brief Receiver ana döngüsünü başlatır.
 *
 * Birden fazla TCP kanalı üzerinden gelen `tap_packet_t` yapılarını dinler,
 * doğrular, sıra numarasına göre kopyaları atlar ve içindeki Ethernet
 * çerçevesini yerel TAP arayüzüne yazar. `select()` sistem çağrısını kullanarak
 * birden fazla soketi izler. Bir kanalda hata oluşursa veya bağlantı koparsa,
 * o kanal kapatılır ve `cleanup` modülünden kaydı silinir.
 * Tüm aktif bağlantılar sonlandığında döngü biter.
 *
 * @param[in] tap_fd        Yerel TAP arayüzünün dosya tanıtıcısı. Gelen paketler buraya yazılır.
 * @param[in,out] sock_fds  Daha önce başarıyla kabul edilmiş ve bağlantı kurulmuş TCP
 * sunucu tarafı soketlerini içeren integer dizisi. Fonksiyon içinde,
 * bir kanalda alım hatası olursa veya bağlantı koparsa ilgili
 * soket kapatılıp dizide INVALID_FD (-1) olarak işaretlenir
 * ve `cleanup_unregister_socket` ile kaydı silinir.
 * @param[in] channel_count İzlenecek kanal sayısı (`sock_fds` dizisinin beklenen boyutu).
 * @return Döngü normal şekilde (tüm bağlantılar kapandığında) sonlanırsa genellikle 0 döner.
 * Bir `select()` hatası gibi kritik bir durumda -1 dönebilir.
 * (Mevcut implementasyonda normal sonlanmada 0, select hatasında -1 döner [cite: receiver.c]).
 */
int receiver_main_loop(int tap_fd,
                       /*const*/ int sock_fds[], // const kaldırıldı çünkü fonksiyon içinde değiştiriliyor
                       int channel_count);

/**
 * @brief Belirtilen soket üzerinden istenen miktarda veriyi almayı garanti eder (blocking).
 *
 * TCP'nin akış tabanlı doğası gereği `recv()` sistem çağrısı istenen tüm veriyi
 * tek seferde okumayabilir. Bu fonksiyon, `len` ile belirtilen tüm verinin
 * alındığından emin olana kadar `recv()` çağrısını tekrarlar.
 *
 * @param[in] sock Veri alınacak soket dosya tanıtıcısı.
 * @param[out] buf Alınan verinin yazılacağı buffer (bellek alanı).
 * @param[in] len  Alınması beklenen verinin bayt cinsinden uzunluğu.
 * @return Başarılı olursa alınan toplam bayt sayısını (bu `len` değerine eşit olacaktır),
 * bir hata oluşursa (örneğin bağlantı kopması veya `recv` hatası) -1 döner. [cite: receiver.c]
 */
ssize_t recvall(int sock, void *buf, size_t len);

#endif // RECEIVER_H
