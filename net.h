/**
 * @file net.h
 * @brief TCP ağ iletişimi için yardımcı fonksiyonların arayüzü.
 *
 * Bu modül, `tapbridge` projesinin TCP istemci ve sunucu soketlerini
 * oluşturması, bağlantıları kabul etmesi ve soketleri belirli ağ
 * arayüzlerine bağlaması için gerekli fonksiyonları içerir.
 *
 * @author Emirhan Çam
 * @date May 15, 2025
 */

#ifndef NET_H
#define NET_H
#include <stdbool.h>
/**
 * @brief Belirtilen hedef IP ve porta bir TCP istemci soketi oluşturur ve bağlanır.
 *
 * Bu fonksiyon, `tapbridge`'in gönderici (sender) modu tarafından alıcıya
 * TCP bağlantıları kurmak için kullanılır. Başarılı bir bağlantı kurulduğunda,
 * oluşturulan soket dosya tanıtıcısı `cleanup` modülüne kaydedilir.
 *
 * @param[in] dst_ip Hedef sunucunun IP adresi (string olarak, örn: "10.1.1.2").
 * @param[in] dst_port Hedef sunucunun port numarası.
 * @return Başarılı olursa yeni oluşturulan ve bağlanan soketin dosya tanıtıcısını,
 * bir hata oluşursa -1 döner. Hata detayları loglanır.
 */
int create_tcp_client_socket(const char *dst_ip, int dst_port);

/**
 * @brief Belirtilen IP ve port üzerinde gelen TCP bağlantılarını dinlemek için bir sunucu soketi oluşturur.
 *
 * Bu fonksiyon, `tapbridge`'in alıcı (receiver) modu tarafından göndericiden
 * gelecek TCP bağlantılarını beklemek için kullanılır. Soket, `SO_REUSEADDR`
 * seçeneği ile ayarlanır. Başarıyla oluşturulup dinlemeye başlandığında,
 * dinleme soketi dosya tanıtıcısı `cleanup` modülüne kaydedilir.
 *
 * @param[in] bind_ip Dinleme yapılacak yerel IP adresi (string olarak, örn: "0.0.0.0" veya belirli bir IP).
 * @param[in] bind_port Dinleme yapılacak yerel port numarası.
 * @return Başarılı olursa yeni oluşturulan dinleme soketinin dosya tanıtıcısını,
 * bir hata oluşursa -1 döner. Hata detayları loglanır.
 */
int create_tcp_server_socket(const char *bind_ip, int bind_port);

/**
 * @brief Bir TCP sunucu dinleme soketi üzerinden gelen yeni bir bağlantıyı kabul eder.
 *
 * Bu fonksiyon, `create_tcp_server_socket` ile oluşturulmuş bir dinleme soketinden
 * gelen bağlantı isteğini kabul eder ve iletişim için yeni bir soket
 * dosya tanıtıcısı döndürür. Bağlantı kuran istemcinin IP adresi ve portu loglanır.
 * Bu yeni bağlantı soketi, çağıran fonksiyon tarafından `cleanup` modülüne
 * ayrıca kaydedilmelidir.
 *
 * @param[in] server_fd Gelen bağlantıları dinleyen sunucu soketinin dosya tanıtıcısı.
 * @return Başarılı olursa yeni kabul edilen bağlantının soket dosya tanıtıcısını,
 * bir hata oluşursa -1 döner. Hata detayları loglanır.
 */
int accept_tcp_connection(int server_fd);

/**
 * @brief Bir soketi belirli bir ağ arayüzüne bağlar (SO_BINDTODEVICE).
 *
 * Bu fonksiyon, belirtilen soketin tüm trafiğinin yalnızca belirtilen ağ
 * arayüzü üzerinden gönderilip alınmasını sağlamak için kullanılır.
 * Özellikle birden fazla ağ arayüzü olan sistemlerde kullanışlıdır.
 *
 * @param[in] sockfd Arayüze bağlanacak soketin dosya tanıtıcısı.
 * @param[in] ifname Bağlantının yapılacağı ağ arayüzünün adı (örn: "eth0", "tap0").
 * @return Başarılı olursa 0, bir hata oluşursa -1 döner. Hata detayları loglanır (WARN seviyesinde).
 */
int bind_socket_on_interface(int sockfd, const char *ifname);

int net_auto_connect(const char *remote_ip);
int net_auto_connect_with_role(const char *local_ip, const char *remote_ip, bool prefer_server);


#endif // NET_H
