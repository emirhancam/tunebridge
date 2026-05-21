/**
 * @file log.h
 * @brief Modüler, formatlı, seviyeli log sistemi arayüzü
 * Written by Emirhan Cam
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_LOG_LINE_LENGTH 2048 ///< Maksimum log mesajı uzunluğu
#define TRUNCATION_SUFFIX " ...[TRUNCATED]" ///< Mesaj sınırı aşılırsa eklenecek ifade

/**
 * @enum LogLevel
 * @brief Log seviyeleri
 */
typedef enum {
    LOG_DEBUG,  ///< Detaylı hata ayıklama bilgisi
    LOG_INFO,   ///< Genel bilgi mesajı
    LOG_WARN,   ///< Uyarı mesajı
    LOG_ERROR,  ///< Hata mesajı
    LOG_FATAL   ///< Ölümcül hata mesajı
} LogLevel;

/**
 * @enum LogFormat
 * @brief Log çıktısı formatları
 */
typedef enum {
    LOG_FORMAT_TABLE, ///< Tablo formatı (varsayılan)
    LOG_FORMAT_CSV,   ///< CSV formatı
    LOG_FORMAT_JSON   ///< JSON formatı
} LogFormat;

/**
 * @brief Log sistemini başlatır (program adı, klasör, seviye, format).
 *
 * @param program_name Log dosyasının adında kullanılacak program adı
 * @param output_path  Log dosyasının yazılacağı klasör
 * @param level        Minimum log seviyesi (örnek: LOG_INFO)
 * @param format       Log formatı (örnek: LOG_FORMAT_JSON)
 */
void log_init(const char *program_name, const char *output_path, LogLevel level, LogFormat format);

/**
 * @brief Log mesajını dosyaya yazar (dahili kullanım için).
 *
 * @param level Log seviyesi
 * @param file Kaynak dosya adı (__FILE__)
 * @param line Satır numarası (__LINE__)
 * @param func Fonksiyon adı (__func__)
 * @param format printf benzeri formatlama
 * @param ... Argümanlar
 */
void log_message_detailed(LogLevel level, const char *file, int line, const char *func, const char *format, ...);

/**
 * @brief Kullanımı kolaylaştırmak için log makrosu
 */
#define LOG(level, format, ...) \
    log_message_detailed(level, __FILE__, __LINE__, __func__, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // LOG_H
