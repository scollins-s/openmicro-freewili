#include "fw2.h"
#include "catalog_parse.h"
#include "platform/diag.h"
#include "store_cmd.h"
#include "store_ui.h"
#include "usbhost/usb_store.h"
#include "pico/stdlib.h"
#include "ff.h"
#include <stdio.h>
#include <string.h>

/*
 * Display-CPU store UI (offline USB MVP).
 * Online CDN download is owned by the MAIN CPU FWSA peer + Bottlenose.
 * Later: OneWili store commands (see docs/bottlenose-store-agent/ONEWILI-STORE-COMMANDS.md).
 */

static store_ui_t s_ui;
static store_cmd_t s_cmd;
static catalog_t s_catalog;
static bool s_catalog_loaded;

static bool load_catalog_from_usb(void) {
    FIL fil;
    if (f_open(&fil, "0:/freewili-store/catalog.json", FA_READ) != FR_OK) {
        return false;
    }
    static char s_json[96 * 1024];
    UINT br = 0;
    FSIZE_t sz = f_size(&fil);
    if (sz >= sizeof(s_json)) sz = sizeof(s_json) - 1;
    if (f_read(&fil, s_json, (UINT)sz, &br) != FR_OK) {
        f_close(&fil);
        return false;
    }
    f_close(&fil);
    s_json[br] = 0;
    if (!catalog_parse(s_json, br, &s_catalog)) {
        DIAG("store: catalog parse failed\n");
        return false;
    }
    s_catalog_loaded = true;
    DIAG("store: catalog v%d apps=%d\n", s_catalog.version, s_catalog.app_count);
    char st[64];
    snprintf(st, sizeof(st), "Catalog: %d apps", s_catalog.app_count);
    store_ui_set_status(&s_ui, st);
    store_ui_show_list(&s_ui);
    return true;
}

static void note_online_unavailable(void) {
    store_ui_set_status(&s_ui, "Online: use main FWSA peer");
    DIAG("store: online download requires main-store-peer + OneWili store cmds\n");
}

int main(void) {
    board_init();
    st7796_init();
    board_backlight_set(1);
    ft6336_init();

    store_ui_init(&s_ui);
    store_cmd_init(&s_cmd);
    store_ui_set_status(&s_ui, "USB store (offline MVP)");
    store_ui_draw(&s_ui, NULL);

    DIAG("store_receiver: display UI — USB catalog only (no Orca on display)\n");
    usb_store_init();

    bool was_mounted = false;
    bool touching = false;

    while (true) {
        usb_store_task();
        bool m = usb_store_mounted();
        if (m != was_mounted) {
            was_mounted = m;
            DIAG("store: USB %s\n", m ? "mounted" : "removed");
            if (m) {
                store_ui_set_status(&s_ui, "USB OK");
                if (!load_catalog_from_usb()) {
                    store_ui_set_status(&s_ui, "No catalog.json on stick");
                }
            } else {
                store_ui_set_status(&s_ui, "USB waiting…");
                s_catalog_loaded = false;
            }
        }

        char get_id[CATALOG_ID_MAX];
        bool want_refresh = false;
        if (store_cmd_poll(&s_cmd, &s_ui, s_catalog_loaded ? &s_catalog : NULL, get_id,
                           sizeof(get_id), &want_refresh)) {
            if (get_id[0]) {
                /* Offline: locate file already on stick; online deferred. */
                const catalog_app_t *app = catalog_find(&s_catalog, get_id);
                if (!app) store_ui_set_status(&s_ui, "Unknown id");
                else note_online_unavailable();
            }
        }
        if (want_refresh) {
            if (m && load_catalog_from_usb()) {
                /* reloaded from USB */
            } else {
                note_online_unavailable();
            }
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
                    /* DOWNLOAD button — online path not on display CPU */
                    note_online_unavailable();
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
