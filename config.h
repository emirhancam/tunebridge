// config.h
#ifndef CONFIG_H
#define CONFIG_H

#include "tunebridge.h" // CHANNEL_COUNT ve MODE_XXX sabitlerini kullanmak için

#define MAX_LINE 128

typedef struct {
    int role;  // MODE_SENDER, MODE_RECEIVER, 998 (duplex)
    const char *local_ips[CHANNEL_COUNT];
    const char *remote_ips[CHANNEL_COUNT];
} config_t;

int read_config(const char *filename, config_t *config);

#endif // CONFIG_H
