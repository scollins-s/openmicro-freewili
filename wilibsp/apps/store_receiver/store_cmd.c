#include "store_cmd.h"

#include "platform/diag.h"
#include "SEGGER_RTT.h"
#include <stdio.h>
#include <string.h>

void store_cmd_init(store_cmd_t *cmd) {
    memset(cmd, 0, sizeof(*cmd));
}

static void print_help(void) {
    DIAG("store cmds: help | list | refresh | get <id>\n");
    DIAG("  offline USB MVP; online needs main FWSA + OneWili store cmds\n");
    DIAG("  firmware apply = BOOTSEL per target CPU (main vs display)\n");
}

static void print_list(const catalog_t *cat) {
    if (!cat || cat->app_count == 0) {
        DIAG("store: empty catalog\n");
        return;
    }
    for (int i = 0; i < cat->app_count; i++) {
        const catalog_app_t *a = &cat->apps[i];
        DIAG("  %s  %s  [%s]%s\n", a->id, a->name, a->kind,
             a->replaces_stock ? " FLASH" : "");
    }
}

bool store_cmd_poll(store_cmd_t *cmd, store_ui_t *ui, const catalog_t *cat,
                    char *out_get_id, size_t out_sz, bool *want_refresh) {
    if (out_get_id && out_sz) out_get_id[0] = 0;
    if (want_refresh) *want_refresh = false;

    char ch;
    while (SEGGER_RTT_Read(0, &ch, 1) == 1) {
        if (ch == '\r') continue;
        if (ch == '\n') {
            cmd->line[cmd->len] = 0;
            char *line = cmd->line;
            while (*line == ' ') line++;
            if (strcmp(line, "help") == 0) {
                print_help();
                if (ui) store_ui_show_help(ui);
            } else if (strcmp(line, "list") == 0) {
                print_list(cat);
                if (ui) store_ui_show_list(ui);
            } else if (strcmp(line, "refresh") == 0) {
                if (want_refresh) *want_refresh = true;
                DIAG("store: refresh requested\n");
            } else if (strncmp(line, "get ", 4) == 0) {
                const char *id = line + 4;
                while (*id == ' ') id++;
                if (out_get_id && out_sz) {
                    snprintf(out_get_id, out_sz, "%s", id);
                }
                DIAG("store: get %s\n", id);
            } else if (line[0]) {
                DIAG("store: unknown '%s' (try help)\n", line);
            }
            cmd->len = 0;
            return out_get_id && out_get_id[0];
        }
        if (cmd->len + 1 < sizeof(cmd->line)) {
            cmd->line[cmd->len++] = ch;
        } else {
            cmd->len = 0;
        }
    }
    return false;
}
