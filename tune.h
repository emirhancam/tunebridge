/**
 * @file tap.h
 * @brief TAP (Terminal Access Point) sanal ağ arayüzü yönetimi için fonksiyon arayüzü.
 *
 * Bu modül, bir TAP sanal ağ arayüzünü açmak, bu arayüzden ham Ethernet
 * çerçevelerini okumak ve bu arayüze ham Ethernet çerçevelerini yazmak
 * için gerekli fonksiyonları sağlar.
 *
 * @note Bu fonksiyonlar genellikle yönetici (root) yetkileri gerektirir.
 *
 * @author Emirhan Çam
 * @date May 15, 2025
 */

#ifndef TUNE_H
#define TUNE_H

#include <stddef.h>     // size_t için
#include <sys/types.h>  // ssize_t için

/**
 * @brief Belirtilen isimde bir TAP sanal ağ arayüzü oluşturur ve açar.
 *
 * Oluşturulan arayüz, Layer 2 (Ethernet) seviyesinde çalışır ve
 * `IFF_NO_PI` (No Packet Information) bayrağı ile paket bilgisi başlığı
 * olmadan ham çerçeveleri işler. Başarılı bir şekilde açıldığında,
 * arayüzün dosya tanıtıcısı `cleanup` modülüne kaydedilir.
 *
 * @param[in] ifname Oluşturulacak TAP arayüzünün adı (örn: "tap0", "mytap").
 * @return Başarılı olursa TAP arayüzünün dosya tanıtıcısını,
 * bir hata oluşursa (örn: /dev/net/tun açılamazsa veya ioctl hatası) -1 döner.
 * Hata detayları loglanır.
 */
int tune_open(const char *ifname);

/**
 * @brief Açık bir TAP arayüzünden ham bir Ethernet çerçevesi okur.
 *
 * Bu fonksiyon genellikle engelleyicidir (blocking), yani arayüzden veri
 * gelene kadar bekler. Okunan veri miktarı ve olası hatalar loglanır.
 *
 * @param[in] fd Veri okunacak TAP arayüzünün dosya tanıtıcısı.
 * @param[out] buf Okunan verinin (Ethernet çerçevesi) yazılacağı buffer.
 * @param[in] len `buf` buffer'ının bayt cinsinden maksimum boyutu.
 * @return Başarılı olursa okunan bayt sayısını, hata oluşursa -1,
 * eğer bağlantı sonlandıysa (EOF) 0 dönebilir.
 */
ssize_t tune_read(int fd, void *buf, size_t len);

/**
 * @brief Açık bir TAP arayüzüne ham bir Ethernet çerçevesi yazar.
 *
 * Yazılan veri miktarı ve olası hatalar loglanır.
 *
 * @param[in] fd Veri yazılacak TAP arayüzünün dosya tanıtıcısı.
 * @param[in] buf Yazılacak veriyi (Ethernet çerçevesi) içeren buffer.
 * @param[in] len Yazılacak verinin bayt cinsinden uzunluğu.
 * @return Başarılı olursa yazılan bayt sayısını (bu `len` değerine eşit olmayabilir),
 * hata oluşursa -1 döner.
 */
ssize_t tune_write(int fd, const void *buf, size_t len);

#endif // TUNE_H
