/**
 * @file cleanup.h
 * @brief Kaynak temizleme modülünün arayüzü.
 *
 * Bu modül, program sonlandığında veya bir sinyal alındığında
 * açılmış olan TAP arayüzlerini ve TCP soketlerini güvenli bir şekilde
 * kapatmak için gerekli fonksiyonları içerir. `atexit` ve sinyal
 * yakalayıcılar aracılığıyla otomatik temizlik sağlar.
 *
 * @author Emirhan Çam
 * @date May 20, 2025
 */

#ifndef CLEANUP_H
#define CLEANUP_H

/**
 * @brief Bir TCP soket dosya tanıtıcısını temizlik için kaydeder.
 *
 * Kaydedilen soketler, `cleanup_run()` çağrıldığında veya program sonlandığında
 * otomatik olarak kapatılır. Aynı fd birden fazla kez kaydedilmeye çalışılırsa
 * işlem yapılmaz (debug logu atılır).
 *
 * @param[in] fd Kaydedilecek soket dosya tanıtıcısı.
 */
void cleanup_register_socket(int fd);

/**
 * @brief Bir TAP arayüzü dosya tanıtıcısını temizlik için kaydeder.
 *
 * Kaydedilen TAP arayüzü, `cleanup_run()` çağrıldığında veya program sonlandığında
 * otomatik olarak kapatılır. Genellikle sadece bir TAP arayüzü takip edilir.
 *
 * @param[in] fd Kaydedilecek TAP arayüzü dosya tanıtıcısı.
 */
void cleanup_register_tap(int fd);

/**
 * @brief Bir TCP soketinin temizlik kaydını siler (INVALID_FD olarak işaretler).
 *
 * Bu fonksiyon, bir soket programın normal akışı içinde manuel olarak
 * kapatıldığında (örneğin, receiver'da bir bağlantı koptuğunda) çağrılmalıdır.
 * Böylece `cleanup_run()` bu soketi tekrar kapatmaya çalışmaz.
 *
 * @param[in] fd Kaydı silinecek (veya geçersiz olarak işaretlenecek) soket dosya tanıtıcısı.
 */
void cleanup_unregister_socket(int fd);

/**
 * @brief Bir TAP arayüzünün temizlik kaydını siler (INVALID_FD olarak işaretler).
 *
 * Bu fonksiyon, TAP arayüzü programın normal akışı içinde manuel olarak
 * kapatılırsa çağrılabilir. Genellikle TAP arayüzü program sonuna kadar
 * açık kalır ve `cleanup_run` tarafından kapatılır.
 *
 * @param[in] fd Kaydı silinecek (veya geçersiz olarak işaretlenecek) TAP arayüzü dosya tanıtıcısı.
 */
void cleanup_unregister_tap(int fd);

/**
 * @brief Temizlik modülünü başlatır.
 *
 * Bu fonksiyon programın başında çağrılmalıdır. `atexit()` ile `cleanup_run()`
 * fonksiyonunu program normal sonlandığında çalışmak üzere kaydeder.
 * Ayrıca, `SIGINT`, `SIGTERM` gibi sinyalleri yakalamak için sinyal
 * yöneticilerini (signal handlers) kurar.
 */
void cleanup_init(void);

/**
 * @brief Kayıtlı tüm kaynakları (TAP ve soketler) temizler (kapatır).
 *
 * Bu fonksiyon, `atexit()` tarafından program normal sonlandığında,
 * yakalanan bir sinyal üzerine `handle_signal()` tarafından veya nadiren
 * manuel olarak çağrılabilir. Birden fazla kez çağrılsa bile temizlik
 * işlemlerini yalnızca bir kere yapar.
 */
void cleanup_run(void);

#endif // CLEANUP_H
