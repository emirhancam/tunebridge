/**
 * @file tapbridge.h
 * @brief TapBridge projesi için temel tanımlamaları ve paylaşılan veri yapılarını içerir.
 *
 * Bu başlık dosyası, projenin farklı modülleri tarafından kullanılan ana veri yapısı
 * olan `tap_packet_t`'yi, çalışma modlarını, kanal ayarlarını ve zaman damgası
 * alma gibi yardımcı fonksiyonları tanımlar.
 * @author Emirhan Çam
 * @date May 15, 2025
 * */

#ifndef TAPBRIDGE_H
#define TAPBRIDGE_H

#include <stdint.h> // Standart integer tipleri için (uint8_t, uint32_t, uint64_t)
#include <stdio.h>  // NULL, size_t (offsetof için dolaylı yoldan gerekebilir)
#include <stdlib.h> // exit (log.c içindeki bazı FATAL durumları için dolaylı olarak)
#include <string.h> // (Kullanılmıyor gibi ama genel başlıklar arasında)
#include <time.h>   // struct timespec, clock_gettime
#include "log.h"    // LOG makrosu ve loglama fonksiyonları için

// Çalışma Modları
/** @def MODE_SENDER
 * @brief Programın gönderici modunda çalışacağını belirtir. */
#define MODE_SENDER   1

/** @def MODE_RECEIVER
 * @brief Programın alıcı modunda çalışacağını belirtir. */
#define MODE_RECEIVER 2

// Paket Boyutları ve Kanal Ayarları
/** @def MAX_PACKET_SIZE
 * @brief TAP arayüzünden okunacak veya TCP üzerinden gönderilecek maksimum payload boyutu.
 * Genellikle bir Ethernet çerçevesinin maksimum boyutuna (MTU + L2 başlıkları vb.) yakın seçilir. */
#define MAX_PACKET_SIZE 2000

/** @def CHANNEL_COUNT
 * @brief Kullanılacak paralel TCP iletişim kanalı sayısı. */
#define CHANNEL_COUNT 3

/** @def BASE_PORT
 * @brief TCP kanalları için kullanılacak temel port numarası.
 * Diğer kanallar için portlar `BASE_PORT + kanal_indeksi` şeklinde hesaplanır. */
#define BASE_PORT 5000

/**
 * @struct tap_packet_t
 * @brief TAP arayüzünden okunan/yazılan verilerin TCP üzerinden iletilirken kullanılan paket yapısı.
 *
 * Bu yapı, ham Ethernet çerçevesini (payload) ve bu çerçevenin doğru bir şekilde
 * işlenmesi ve çoğaltılmasının engellenmesi için gerekli meta verileri içerir.
 */
typedef struct {
    uint8_t  channel_id;                  /**< @brief Paketin gönderildiği veya alındığı kanalın kimliği (0, 1, ..., CHANNEL_COUNT-1). */
    uint32_t seq_no;                      /**< @brief Her kanal için artan paket sıra numarası. Kopyaları ve kayıpları tespit etmek için kullanılır. */
    uint64_t timestamp_us;               /**< @brief Paketin gönderildiği veya alındığı zamanki mikrosaniye cinsinden zaman damgası. */
    uint32_t length;                     /**< @brief `payload` alanındaki geçerli veri uzunluğu (Ethernet çerçevesinin boyutu). */
    uint8_t  payload[MAX_PACKET_SIZE];   /**< @brief Asıl TAP verisini (ham Ethernet çerçevesi) içeren buffer. */
} tap_packet_t;

/**
 * @brief Mevcut zamanı mikrosaniye (µs) cinsinden döndürür.
 *
 * `CLOCK_REALTIME` kullanarak yüksek çözünürlüklü bir zaman damgası alır.
 * Bu fonksiyon, `tap_packet_t` yapısındaki `timestamp_us` alanını doldurmak
 * için kullanılır.
 *
 * @return Mikrosaniye cinsinden 64-bitlik zaman damgası.
 */
static inline uint64_t get_timestamp_us() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts); // Sistem saatini al
    return (uint64_t)(ts.tv_sec * 1000000ULL + ts.tv_nsec / 1000);
}

#endif // TAPBRIDGE_H
