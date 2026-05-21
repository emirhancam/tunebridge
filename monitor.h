/**
 * @file monitor.h
 * @brief TCP kanallarının durumunu izleyen monitör modülünün arayüzü.
 *
 * Bu modül, `tapbridge` projesindeki aktif TCP iletişim kanallarının
 * periyodik olarak sağlık durumunu kontrol etmek ve bağlantı kayıplarını
 * loglamak için kullanılır.
 *
 * @author Emirhan Çam
 * @date May 20, 2025
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <signal.h>  // <-- Bu eksik!
#define CHANNEL_COUNT 3  // Gerekirse dışarıdan da alınabilir

extern volatile sig_atomic_t stop_flags[CHANNEL_COUNT];

/**
 * @brief Verilen TCP kanallarının bağlantı durumunu kontrol eder ve loglar.
 *
 * Bu fonksiyon, `sender_main_loop` tarafından periyodik olarak çağrılarak
 * her bir kanalın hala aktif olup olmadığını kontrol eder. `is_channel_alive`
 * (monitor.c içinde tanımlı, bu header'da olmayan yardımcı fonksiyon)
 * fonksiyonunu kullanarak her bir soket için 0 byte'lık bir mesaj göndermeyi
 * dener (MSG_NOSIGNAL ile). Bağlantı kopmuşsa veya soket geçerli değilse
 * (örneğin -1 ise) "KAYIP" olarak, aksi halde "AKTİF" olarak loglar.
 *
 * @param[in] channel_fds Kontrol edilecek TCP soket dosya tanıtıcılarını içeren dizi.
 * Bu dizideki `-1` gibi geçersiz değerler de güvenle işlenir
 * (genellikle "KAYIP" olarak sonuçlanır).
 * @param[in] channel_count `channel_fds` dizisindeki eleman sayısı.
 *
 * @todo Yeniden bağlanmayı deneme mekanizması eklenmeli.
 * @todo Kanallar için RTT (Round Trip Time) ölçümü ve gecikme analizi eklenebilir.
 */
void monitor_check_channels(const int *channel_fds, int channel_count);

#endif // MONITOR_H
