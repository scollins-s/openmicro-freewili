#ifndef STORE_UI_H
#define STORE_UI_H

#include "catalog_parse.h"
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    STORE_UI_LIST = 0,
    STORE_UI_DETAIL,
    STORE_UI_PROGRESS,
    STORE_UI_HELP,
} store_ui_screen_t;

typedef enum {
    STORE_UI_TAP_NONE = 0,
    STORE_UI_TAP_DOWNLOAD,
    STORE_UI_TAP_POWER_OFF,
    STORE_UI_TAP_EXIT, /* return to persistent flash launcher (RAM apps) */
} store_ui_tap_result_t;

typedef struct store_ui {
    store_ui_screen_t screen;
    int list_top;       /* scroll offset */
    int selected;       /* index into catalog */
    uint32_t bytes_done;
    uint32_t bytes_total;
    char status[64];
    bool dirty;
} store_ui_t;

void store_ui_init(store_ui_t *ui);
void store_ui_set_status(store_ui_t *ui, const char *msg);
void store_ui_set_progress(store_ui_t *ui, uint32_t done, uint32_t total);
void store_ui_show_list(store_ui_t *ui);
void store_ui_show_detail(store_ui_t *ui);
void store_ui_show_help(store_ui_t *ui);
void store_ui_show_progress(store_ui_t *ui);

/* Redraw if dirty. */
void store_ui_draw(store_ui_t *ui, const catalog_t *cat);

/*
 * Handle a touch tap at panel coords.
 * DOWNLOAD → caller should fetch the selected app.
 * POWER_OFF → caller should call store_ui_standby_until_tap().
 * EXIT → caller should reboot to the persistent flash launcher (RAM builds).
 */
store_ui_tap_result_t store_ui_on_tap(store_ui_t *ui, const catalog_t *cat,
                                      uint16_t x, uint16_t y);

/* Blank screen, backlight off; wake on tap or RTT key (sleeps between polls). */
void store_ui_standby_until_tap(store_ui_t *ui);

#endif
