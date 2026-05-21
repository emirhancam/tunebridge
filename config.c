// config.c
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *strdup_strip(const char *src) {
    while (*src == ' ' || *src == '\t') src++;
    size_t len = strlen(src);
    while (len > 0 && (src[len - 1] == '\n' || src[len - 1] == '\r' || src[len - 1] == ' ' || src[len - 1] == '\t')) len--;
    char *out = malloc(len + 1);
    if (!out) return NULL;
    strncpy(out, src, len);
    out[len] = '\0';
    return out;
}

int read_config(const char *filename, config_t *config) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        perror("Config dosyası açılamadı");
        return -1;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fp)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;

        *eq = '\0';
        char *key = strdup_strip(line);
        char *val = strdup_strip(eq + 1);

        //paranoyakca kodlama.
        if (!key || !val) {
            LOG(LOG_FATAL, "Yapılandırma satırında bellek ayrılamadı (key/val NULL)");
            free(key);  // ikisinden biri NULL değilse sızıntı olmasın diye
            free(val);
            fclose(fp);
            return -1;
        }

        if (strcmp(key, "role") == 0) {
            if (strcmp(val, "sender") == 0) config->role = MODE_SENDER;
            else if (strcmp(val, "receiver") == 0) config->role = MODE_RECEIVER;
            else if (strcmp(val, "duplex") == 0) config->role = 998;
        }

        for (int i = 0; i < CHANNEL_COUNT; ++i) {
            char key_local[32], key_remote[32];
            snprintf(key_local, sizeof(key_local), "local_ip%d", i);
            snprintf(key_remote, sizeof(key_remote), "remote_ip%d", i);

            if (strcmp(key, key_local) == 0) {
                config->local_ips[i] = strdup(val);
            } else if (strcmp(key, key_remote) == 0) {
                config->remote_ips[i] = strdup(val);
            }
        }

        free(key);
        free(val);
    }

    fclose(fp);
    return 0;
}
