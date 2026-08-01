#ifndef STORE_CMD_H
#define STORE_CMD_H

#include "catalog_parse.h"
#include "store_ui.h"
#include <stdbool.h>

typedef struct store_cmd {
    char line[128];
    size_t len;
} store_cmd_t;

void store_cmd_init(store_cmd_t *cmd);

/*
 * Poll RTT down-buffer for console commands.
 * Supported: help | list | refresh | get <id> | help
 * Returns true if caller should refresh catalog from CDN.
 * If out_get_id is set and command is get, copies id (caller downloads).
 */
bool store_cmd_poll(store_cmd_t *cmd, store_ui_t *ui, const catalog_t *cat,
                    char *out_get_id, size_t out_sz, bool *want_refresh);

#endif
