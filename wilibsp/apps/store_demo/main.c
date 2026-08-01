#include "fw2.h"
#include "catalog_embed.h"
#include "catalog_parse.h"
#include "platform/diag.h"
#include "store_cmd.h"
#include "store_ui.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

/*
 * Display-CPU store UI with catalog JSON compiled in (no USB / no online).
 * Browse-only demo: download actions are stubbed.
 */

static store_ui_t s_ui;
static store_cmd_t s_cmd;
static catalog_t s_catalog;
static bool s_catalog_loaded;

static bool load_embedded_catalog(void) {
    if (!catalog_parse(k_catalog_json, k_catalog_json_len, &s_catalog)) {
        DIAG("store_demo: catalog parse failed\n");
        s_catalog_loaded = false;
        return false;
    }
    s_catalog_loaded = true;
    DIAG("store_demo: catalog v%d apps=%d (embedded)\n",
         s_catalog.version, s_catalog.app_count);
    char st[64];
    snprintf(st, sizeof(st), "Built-in: %d apps", s_catalog.app_count);
    store_ui_set_status(&s_ui, st);
    store_ui_show_list(&s_ui);
    return true;
}

static void note_demo_no_download(void) {
    store_ui_set_status(&s_ui, "Demo: catalog only (no download)");
    DIAG("store_demo: download not available in embedded catalog build\n");
}

int main(void) {
    board_init();
    st7796_init();
    board_backlight_set(1);
    ft6336_init();

    store_ui_init(&s_ui);
    store_cmd_init(&s_cmd);
    store_ui_set_status(&s_ui, "Built-in catalog");
    store_ui_draw(&s_ui, NULL);

    DIAG("store_demo: display UI — embedded catalog JSON\n");
    if (!load_embedded_catalog()) {
        store_ui_set_status(&s_ui, "Embedded catalog parse failed");
    }

    bool touching = false;

    while (true) {
        char get_id[CATALOG_ID_MAX];
        bool want_refresh = false;
        if (store_cmd_poll(&s_cmd, &s_ui, s_catalog_loaded ? &s_catalog : NULL, get_id,
                           sizeof(get_id), &want_refresh)) {
            if (get_id[0]) {
                const catalog_app_t *app = catalog_find(&s_catalog, get_id);
                if (!app) store_ui_set_status(&s_ui, "Unknown id");
                else note_demo_no_download();
            }
        }
        if (want_refresh) {
            load_embedded_catalog();
        }

        uint16_t tx = 0, ty = 0;
        if (ft6336_poll(&tx, &ty)) {
            if (!touching) {
                touching = true;
                store_ui_tap_result_t tap =
                    store_ui_on_tap(&s_ui, s_catalog_loaded ? &s_catalog : NULL, tx, ty);
                if (tap == STORE_UI_TAP_POWER_OFF) {
                    store_ui_standby_until_tap(&s_ui);
                    touching = false;
                } else if (tap == STORE_UI_TAP_DOWNLOAD) {
                    note_demo_no_download();
                    store_ui_show_help(&s_ui);
                }
            }
        } else {
            touching = false;
        }

        store_ui_draw(&s_ui, s_catalog_loaded ? &s_catalog : NULL);
        sleep_ms(16);
    }
}
