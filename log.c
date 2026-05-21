
/**
 * @file log.c
 * @brief Log sisteminin fonksiyonel implementasyonu
 * Written by Emirhan Cam
 */

#include "log.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdarg.h>
#include <unistd.h>

static LogLevel current_level = LOG_INFO;
static LogFormat current_format = LOG_FORMAT_TABLE;
static FILE *log_file = NULL;
static char log_filename[256];
static const char *program_name = "program";
static char log_directory[256] = "./";
static int last_log_day = -1;

static const char *level_names[] = {
    "DEBUG", "INFO", "WARN", "ERROR", "FATAL"
};

void log_init(const char *name, const char *path, LogLevel level, LogFormat format) {
    program_name = name;
    current_level = level;
    current_format = format;
    last_log_day = -1;

    if (path && strlen(path) > 0) {
        snprintf(log_directory, sizeof(log_directory), "%s", path);
    } else {
        snprintf(log_directory, sizeof(log_directory), ".");
    }
}

void log_message_detailed(LogLevel level, const char *file, int line, const char *func, const char *format, ...) {
    if (level < current_level) return;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    struct tm *t = localtime(&tv.tv_sec);

    char time_buf[64];
    snprintf(time_buf, sizeof(time_buf),
             "%02d:%02d:%02d.%03ld",
             t->tm_hour, t->tm_min, t->tm_sec, tv.tv_usec / 1000);

    if (t->tm_mday != last_log_day) {
        last_log_day = t->tm_mday;

        if (log_file) fclose(log_file);

        const char *extension = ".log";
        if (current_format == LOG_FORMAT_JSON) extension = ".json";
        else if (current_format == LOG_FORMAT_CSV) extension = ".csv";

        int n = snprintf(log_filename, sizeof(log_filename),
                         "%s/%s_%d_%02d_%02d%s",
                         log_directory,
                         program_name,
                         t->tm_year + 1900,
                         t->tm_mday,
                         t->tm_mon + 1,
                         extension);

        if (n < 0 || n >= (int)sizeof(log_filename)) {
            fprintf(stderr, "log filename too long\n");
            exit(EXIT_FAILURE);
        }

        int file_exists = access(log_filename, F_OK) == 0;

        log_file = fopen(log_filename, "a");
        if (!log_file) {
            perror("log file reopen failed");
            exit(EXIT_FAILURE);
        }

        if (!file_exists && current_format == LOG_FORMAT_TABLE) {
            fprintf(log_file,
                    "| %-15s | %-8s | %-10s | %-10s | %-15s | %s |\n",
                    "TIME", "LEVEL", "TIMESTAMP", "FILE", "FUNC:LINE", "MESSAGE");
            fflush(log_file);
        }
    }

    const char *filename = strrchr(file, '/');
    filename = filename ? filename + 1 : file;

    char msg_buffer[MAX_LOG_LINE_LENGTH];
    va_list args;
    va_start(args, format);
    int written = vsnprintf(msg_buffer, sizeof(msg_buffer), format, args);
    va_end(args);

    if (written >= MAX_LOG_LINE_LENGTH) {
        size_t suffix_len = strlen(TRUNCATION_SUFFIX);
        if (MAX_LOG_LINE_LENGTH > suffix_len) {
            memcpy(&msg_buffer[MAX_LOG_LINE_LENGTH - suffix_len - 1], TRUNCATION_SUFFIX, suffix_len);
            msg_buffer[MAX_LOG_LINE_LENGTH - 1] = '\0';
        }
    }

    switch (current_format) {
        case LOG_FORMAT_TABLE: {
            char func_line[64];
            snprintf(func_line, sizeof(func_line), "%s():%d", func, line);
            fprintf(log_file, "| %-15s | %-8s | %-10ld | %-10s | %-15s | %s |\n",
                    time_buf, level_names[level], tv.tv_sec, filename, func_line, msg_buffer);
            break;
        }
        case LOG_FORMAT_CSV:
            fprintf(log_file, "\"%s\",\"%s\",%ld,\"%s\",\"%s\",\"%d\",\"%s\"\n",
                    time_buf, level_names[level], tv.tv_sec, filename, func, line, msg_buffer);
            break;
        case LOG_FORMAT_JSON:
            fprintf(log_file,
                    "{ \"time\": \"%s\", \"level\": \"%s\", \"timestamp\": %ld, \"file\": \"%s\", \"function\": \"%s\", \"line\": %d, \"message\": \"%s\" }\n",
                    time_buf, level_names[level], tv.tv_sec, filename, func, line, msg_buffer);
            break;
    }

    fflush(log_file);
}
