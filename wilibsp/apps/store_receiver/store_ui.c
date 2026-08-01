#include "store_ui.h"

#include "fw2.h"
#include "platform/diag.h"
#include "pico/stdlib.h"
#include "store_panel.h"
#include "SEGGER_RTT.h"
#include <stdio.h>
#include <string.h>

#ifndef STORE_UI_HAS_EXIT
#define STORE_UI_HAS_EXIT 0
#endif

#define COL_FG   0xFFFF
#define COL_ERR  0xF800
#define COL_WARN 0xFFE0
#define COL_DIM  PANEL_COL_MUTED

#define OFF_BTN_X  400
#define OFF_BTN_Y  4
#define OFF_BTN_W  72
#define OFF_BTN_H  24

/* EXIT sits left of OFF when building the no_flash RAM store. */
#define EXIT_BTN_X  320
#define EXIT_BTN_Y  4
#define EXIT_BTN_W  72
#define EXIT_BTN_H  24

/* 2x2 card grid with side arrows for paging (480x320) */
#define GRID_COLS   2
#define GRID_ROWS   2
#define GRID_PAGE   (GRID_COLS * GRID_ROWS)  /* 4 cards per screen */
#define ARROW_W     28
#define GRID_X0     ARROW_W
#define GRID_Y0     36
#define GRID_GAP    8
#define GRID_CARD_W 208
#define GRID_CARD_H 118

static panel_rect_t grid_cell_rect(int slot) {
    int col = slot % GRID_COLS;
    int row = slot / GRID_COLS;
    panel_rect_t r = {
        GRID_X0 + col * (GRID_CARD_W + GRID_GAP),
        GRID_Y0 + row * (GRID_CARD_H + GRID_GAP),
        GRID_CARD_W,
        GRID_CARD_H,
    };
    return r;
}

static int grid_hit_slot(uint16_t x, uint16_t y) {
    for (int slot = 0; slot < GRID_PAGE; slot++) {
        panel_rect_t r = grid_cell_rect(slot);
        if (x >= r.x && x < r.x + r.w && y >= r.y && y < r.y + r.h) return slot;
    }
    return -1;
}

static bool tap_in_arrow_zone(uint16_t x, uint16_t y, bool left) {
    int y0 = GRID_Y0;
    int y1 = GRID_Y0 + GRID_ROWS * GRID_CARD_H + (GRID_ROWS - 1) * GRID_GAP;
    if (y < y0 || y >= y1) return false;
    if (left) return x < ARROW_W;
    return x >= (ST7796_W - ARROW_W);
}

static void sync_page_to_selection(store_ui_t *ui) {
    if (ui->selected < 0) ui->selected = 0;
    ui->list_top = (ui->selected / GRID_PAGE) * GRID_PAGE;
}

static void page_by(store_ui_t *ui, const catalog_t *cat, int delta_pages) {
    if (!cat || cat->app_count <= GRID_PAGE) return;
    int pages = (cat->app_count + GRID_PAGE - 1) / GRID_PAGE;
    int page = ui->list_top / GRID_PAGE;
    page += delta_pages;
    while (page < 0) page += pages;
    while (page >= pages) page -= pages;
    ui->list_top = page * GRID_PAGE;
    ui->selected = ui->list_top;
    ui->dirty = true;
}

void store_ui_init(store_ui_t *ui) {
    memset(ui, 0, sizeof(*ui));
    ui->screen = STORE_UI_LIST;
    ui->dirty = true;
    snprintf(ui->status, sizeof(ui->status), "Waiting USB…");
}

void store_ui_set_status(store_ui_t *ui, const char *msg) {
    snprintf(ui->status, sizeof(ui->status), "%s", msg ? msg : "");
    ui->dirty = true;
}

void store_ui_set_progress(store_ui_t *ui, uint32_t done, uint32_t total) {
    ui->bytes_done = done;
    ui->bytes_total = total;
    ui->screen = STORE_UI_PROGRESS;
    ui->dirty = true;
}

void store_ui_show_list(store_ui_t *ui) {
    ui->screen = STORE_UI_LIST;
    ui->dirty = true;
}
void store_ui_show_detail(store_ui_t *ui) {
    ui->screen = STORE_UI_DETAIL;
    ui->dirty = true;
}
void store_ui_show_help(store_ui_t *ui) {
    ui->screen = STORE_UI_HELP;
    ui->dirty = true;
}
void store_ui_show_progress(store_ui_t *ui) {
    ui->screen = STORE_UI_PROGRESS;
    ui->dirty = true;
}

static void draw_power_btn(void) {
    st7796_fill_rect(OFF_BTN_X, OFF_BTN_Y, OFF_BTN_W, OFF_BTN_H, COL_ERR);
    st7796_draw_text(OFF_BTN_X + 18, OFF_BTN_Y + 6, 1, COL_FG, COL_ERR, "OFF");
}

#if STORE_UI_HAS_EXIT
static void draw_exit_btn(void) {
    st7796_fill_rect(EXIT_BTN_X, EXIT_BTN_Y, EXIT_BTN_W, EXIT_BTN_H, 0x001F);
    st7796_draw_text(EXIT_BTN_X + 12, EXIT_BTN_Y + 6, 1, COL_FG, 0x001F, "EXIT");
}

static bool tap_in_exit_btn(uint16_t x, uint16_t y) {
    return x >= EXIT_BTN_X && x < (EXIT_BTN_X + EXIT_BTN_W) &&
           y >= EXIT_BTN_Y && y < (EXIT_BTN_Y + EXIT_BTN_H);
}
#endif

static bool tap_in_power_btn(uint16_t x, uint16_t y) {
    return x >= OFF_BTN_X && x < (OFF_BTN_X + OFF_BTN_W) &&
           y >= OFF_BTN_Y && y < (OFF_BTN_Y + OFF_BTN_H);
}

static void draw_header(const char *title) {
    st7796_fill_rect(0, 0, ST7796_W, 32, 0x3186);
    st7796_draw_text(8, 8, 2, COL_FG, 0x3186, title);
#if STORE_UI_HAS_EXIT
    draw_exit_btn();
#endif
    draw_power_btn();
}

static void draw_footer(const char *msg) {
    st7796_fill_rect(0, ST7796_H - 24, ST7796_W, 24, 0x3186);
    st7796_draw_text(8, ST7796_H - 18, 1, COL_WARN, 0x3186, msg);
}

static void clamp_selection(store_ui_t *ui, const catalog_t *cat) {
    if (!cat || cat->app_count <= 0) {
        ui->selected = 0;
        return;
    }
    if (ui->selected < 0) ui->selected = 0;
    if (ui->selected >= cat->app_count) ui->selected = cat->app_count - 1;
}

static void draw_list(store_ui_t *ui, const catalog_t *cat) {
    st7796_fill_screen(PANEL_COL_PAGE);
    draw_header("FW2 STORE");

    clamp_selection(ui, cat);
    sync_page_to_selection(ui);

    if (!cat || cat->app_count == 0) {
        st7796_draw_text(40, 120, 1, COL_DIM, PANEL_COL_PAGE, "No catalog yet.");
        st7796_draw_text(40, 140, 1, COL_DIM, PANEL_COL_PAGE, "Insert USB or use built-in demo.");
    } else {
        for (int slot = 0; slot < GRID_PAGE; slot++) {
            int idx = ui->list_top + slot;
            if (idx >= cat->app_count) break;
            panel_rect_t cell = grid_cell_rect(slot);
            panel_draw_card(cell, &cat->apps[idx], idx == ui->selected);
        }

        int pages = (cat->app_count + GRID_PAGE - 1) / GRID_PAGE;
        int page = ui->list_top / GRID_PAGE;
        int mid_y = GRID_Y0 + (GRID_ROWS * GRID_CARD_H + (GRID_ROWS - 1) * GRID_GAP) / 2 - 8;

        if (cat->app_count > GRID_PAGE) {
            st7796_draw_text(8, mid_y, 2, PANEL_COL_ACCENT, PANEL_COL_PAGE, "<");
            st7796_draw_text(ST7796_W - 20, mid_y, 2, PANEL_COL_ACCENT, PANEL_COL_PAGE, ">");
        }

        char pos[40];
        snprintf(pos, sizeof(pos), "<%d/%d>  TAP=OPEN  HELP>",
                 page + 1, pages > 0 ? pages : 1);
        st7796_draw_text(8, ST7796_H - 44, 1, PANEL_COL_ACCENT, PANEL_COL_PAGE, pos);
    }
    draw_footer(ui->status);
}

static void draw_detail(store_ui_t *ui, const catalog_t *cat) {
    st7796_fill_screen(PANEL_COL_PAGE);
    draw_header("APP DETAIL");

    if (!cat || ui->selected < 0 || ui->selected >= cat->app_count) {
        st7796_draw_text(8, 60, 1, COL_ERR, PANEL_COL_PAGE, "Invalid selection");
    } else {
        const catalog_app_t *a = &cat->apps[ui->selected];
        panel_rect_t card = {20, 40, 440, 150};
        panel_fill_rounded(card, 10, PANEL_COL_CARD);
        panel_draw_border(card, 10, 2, PANEL_COL_BORDER);

        st7796_draw_text(32, 52, 2, PANEL_COL_TEXT, PANEL_COL_CARD, a->name);
        const char *badge = a->cpu_badge[0] ? a->cpu_badge : a->kind;
        if (badge[0]) panel_draw_badge(32, 78, badge, PANEL_COL_BADGE, PANEL_COL_BADGE_FG);

        char line[80];
        snprintf(line, sizeof(line), "ID=%s", a->id);
        st7796_draw_text(32, 100, 1, PANEL_COL_MUTED, PANEL_COL_CARD, line);
        if (a->version[0]) {
            snprintf(line, sizeof(line), "V%s", a->version);
            st7796_draw_text(32, 116, 1, PANEL_COL_MUTED, PANEL_COL_CARD, line);
        }
        if (a->replaces_stock) {
            st7796_draw_text(280, 78, 1, COL_ERR, PANEL_COL_CARD, "REPLACES STOCK");
        }
        const char *desc = a->description[0] ? a->description : "No description.";
        panel_draw_text_wrapped(32, 136, 410, 1, 3, PANEL_COL_TEXT, PANEL_COL_CARD, desc);

        if (a->has_artifact) {
            snprintf(line, sizeof(line), "FILE: %s",
                     a->art.filename[0] ? a->art.filename : a->art.url);
            st7796_draw_text(20, 200, 1, PANEL_COL_MUTED, PANEL_COL_PAGE, line);
        }

        st7796_fill_rect(40, 230, 180, 40, 0x0480);
        st7796_draw_text(60, 242, 2, COL_FG, 0x0480, "DOWNLOAD");
        st7796_fill_rect(250, 230, 140, 40, 0x4208);
        st7796_draw_text(280, 242, 2, COL_FG, 0x4208, "BACK");
    }
    draw_footer(ui->status);
}

static void draw_progress(store_ui_t *ui) {
    st7796_fill_screen(PANEL_COL_PAGE);
    draw_header("DOWNLOADING");
    char line[64];
    if (ui->bytes_total > 0) {
        unsigned pct = (unsigned)((ui->bytes_done * 100u) / ui->bytes_total);
        snprintf(line, sizeof(line), "%u%%  %u / %u", pct, (unsigned)ui->bytes_done,
                 (unsigned)ui->bytes_total);
    } else {
        snprintf(line, sizeof(line), "%u bytes…", (unsigned)ui->bytes_done);
    }
    st7796_draw_text(8, 80, 2, PANEL_COL_TEXT, PANEL_COL_PAGE, line);
    int bar_w = 400;
    st7796_fill_rect(40, 140, bar_w, 24, PANEL_COL_BORDER);
    int fill = 0;
    if (ui->bytes_total > 0) {
        fill = (int)((ui->bytes_done * (uint32_t)bar_w) / ui->bytes_total);
        if (fill > bar_w) fill = bar_w;
    }
    if (fill > 0) st7796_fill_rect(40, 140, fill, 24, PANEL_COL_ACCENT);
    draw_footer(ui->status);
}

static void draw_help(store_ui_t *ui) {
    st7796_fill_screen(PANEL_COL_PAGE);
    draw_header("HELP / APPLY");
    st7796_draw_text(8, 48, 1, PANEL_COL_TEXT, PANEL_COL_PAGE, "1. Tap a card to open detail");
    st7796_draw_text(8, 64, 1, PANEL_COL_TEXT, PANEL_COL_PAGE, "2. Tap < > arrows to flip pages");
    st7796_draw_text(8, 80, 1, PANEL_COL_TEXT, PANEL_COL_PAGE, "3. Online download: MAIN + Bottlenose");
    st7796_draw_text(8, 96, 1, PANEL_COL_TEXT, PANEL_COL_PAGE, "4. Firmware: BOOTSEL per target CPU");
#if STORE_UI_HAS_EXIT
    st7796_draw_text(8, 112, 1, COL_WARN, PANEL_COL_PAGE, "RAM store — EXIT returns to launcher.");
#else
    st7796_draw_text(8, 112, 1, COL_WARN, PANEL_COL_PAGE, "No Orca on display. No RAM-load.");
#endif
    st7796_draw_text(8, 136, 1, COL_DIM, PANEL_COL_PAGE, "RTT: help | list | refresh | get <id>");
    st7796_draw_text(8, 152, 1, COL_DIM, PANEL_COL_PAGE, "OFF (top-right): sleep; tap to wake");
    st7796_fill_rect(40, 220, 140, 40, 0x4208);
    st7796_draw_text(70, 232, 2, COL_FG, 0x4208, "BACK");
    draw_footer(ui->status);
}

void store_ui_draw(store_ui_t *ui, const catalog_t *cat) {
    if (!ui || !ui->dirty) return;
    switch (ui->screen) {
    case STORE_UI_LIST:     draw_list(ui, cat); break;
    case STORE_UI_DETAIL:   draw_detail(ui, cat); break;
    case STORE_UI_PROGRESS: draw_progress(ui); break;
    case STORE_UI_HELP:     draw_help(ui); break;
    }
    ui->dirty = false;
}

store_ui_tap_result_t store_ui_on_tap(store_ui_t *ui, const catalog_t *cat,
                                      uint16_t x, uint16_t y) {
    if (!ui) return STORE_UI_TAP_NONE;

#if STORE_UI_HAS_EXIT
    if (tap_in_exit_btn(x, y)) {
        return STORE_UI_TAP_EXIT;
    }
#endif

    if (tap_in_power_btn(x, y)) {
        return STORE_UI_TAP_POWER_OFF;
    }

    if (ui->screen == STORE_UI_LIST) {
        if (y >= ST7796_H - 44 && x > 360) {
            store_ui_show_help(ui);
            return STORE_UI_TAP_NONE;
        }
        if (!cat || cat->app_count == 0) return STORE_UI_TAP_NONE;

        clamp_selection(ui, cat);
        sync_page_to_selection(ui);

        if (cat->app_count > GRID_PAGE) {
            if (tap_in_arrow_zone(x, y, true)) {
                page_by(ui, cat, -1);
                return STORE_UI_TAP_NONE;
            }
            if (tap_in_arrow_zone(x, y, false)) {
                page_by(ui, cat, +1);
                return STORE_UI_TAP_NONE;
            }
        }

        int slot = grid_hit_slot(x, y);
        if (slot >= 0) {
            int idx = ui->list_top + slot;
            if (idx < cat->app_count) {
                ui->selected = idx;
                store_ui_show_detail(ui);
            }
            return STORE_UI_TAP_NONE;
        }

        /* Header (not EXIT/OFF): next page of 4 */
#if STORE_UI_HAS_EXIT
        if (y < 32 && x < EXIT_BTN_X && cat->app_count > GRID_PAGE) {
#else
        if (y < 32 && x < OFF_BTN_X && cat->app_count > GRID_PAGE) {
#endif
            page_by(ui, cat, +1);
        }
        return STORE_UI_TAP_NONE;
    }

    if (ui->screen == STORE_UI_DETAIL) {
        if (y >= 230 && y <= 270) {
            if (x >= 40 && x <= 220) {
                return STORE_UI_TAP_DOWNLOAD;
            }
            if (x >= 250 && x <= 390) {
                store_ui_show_list(ui);
            }
        }
        return STORE_UI_TAP_NONE;
    }

    if (ui->screen == STORE_UI_HELP) {
        if (y >= 220 && y <= 260 && x >= 40 && x <= 180) {
            store_ui_show_list(ui);
        }
        return STORE_UI_TAP_NONE;
    }

    return STORE_UI_TAP_NONE;
}

void store_ui_standby_until_tap(store_ui_t *ui) {
    DIAG("store: standby — tap screen or RTT key to wake\n");
    st7796_fill_screen(0x0000);
    board_backlight_set(0);

    uint16_t x = 0, y = 0;

    /* Wait for current press to end (cap 2s if touch stuck reporting down). */
    absolute_time_t t0 = get_absolute_time();
    while (ft6336_poll(&x, &y)) {
        sleep_ms(20);
        if (absolute_time_diff_us(t0, get_absolute_time()) > 2000000) {
            DIAG("store: standby release timeout (touch stuck?)\n");
            break;
        }
    }
    sleep_ms(80);

    /* Idle until tap or RTT input — sleep between polls (no 250 MHz busy-spin). */
    for (;;) {
        if (ft6336_poll(&x, &y)) break;
        if (SEGGER_RTT_HasKey()) {
            DIAG("store: wake from RTT\n");
            break;
        }
        sleep_ms(50);
    }

    /* Wait for release so the wake tap does not immediately re-trigger UI. */
    t0 = get_absolute_time();
    while (ft6336_poll(&x, &y)) {
        sleep_ms(20);
        if (absolute_time_diff_us(t0, get_absolute_time()) > 2000000) break;
    }

    board_backlight_set(1);
    if (ui) ui->dirty = true;
    DIAG("store: wake from standby\n");
}
