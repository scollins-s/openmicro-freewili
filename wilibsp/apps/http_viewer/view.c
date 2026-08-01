#include "view.h"

#include "config.h"
#include "fw2.h"
#include "platform/diag.h"
#include <stdio.h>
#include <string.h>

#define COL_FG   0xFFFF
#define COL_BG   0x0000
#define COL_HDR  0x3186
#define COL_BTN  0x4208
#define COL_ACC  0x07E0
#define COL_ERR  0xF800
#define COL_WARN 0xFFE0

#define HDR_H    24
#define META_H   24
#define FOOT_H   48
#define BODY_Y   (HDR_H + META_H)
#define BODY_H   (ST7796_H - BODY_Y - FOOT_H)

#define BTN_Y    (ST7796_H - FOOT_H + 8)
#define BTN_H    32
#define BTN_W    100
#define BTN_GAP  12
#define BTN0_X   16

static void draw_btn(int x, const char *label) {
    st7796_fill_rect(x, BTN_Y, BTN_W, BTN_H, COL_BTN);
    st7796_draw_text(x + 12, BTN_Y + 10, 1, COL_FG, COL_BTN, label);
}

void view_init(view_state_t *v) {
    memset(v, 0, sizeof(*v));
    v->dirty = true;
}

void view_enter_standby(view_state_t *v) {
    DIAG("http_viewer: standby (backlight off)\n");
    st7796_fill_screen(COL_BG);
    board_backlight_set(0);
    v->asleep = true;
    v->dirty = false;
}

void view_leave_standby(view_state_t *v) {
    board_backlight_set(1);
    v->asleep = false;
    v->dirty = true;
    DIAG("http_viewer: wake\n");
}

void view_draw(view_state_t *v, const http_net_t *net, const response_store_t *store) {
    if (!v || v->asleep) return;

    st7796_fill_screen(COL_BG);

    /* Header */
    st7796_fill_rect(0, 0, ST7796_W, HDR_H, COL_HDR);
    st7796_draw_text(8, 6, 1, COL_FG, COL_HDR, "HTTP VIEWER");
    const char *wifi = (!net->wifi_known) ? "Wi-Fi: ?" :
                       (net->wifi_ok ? "Wi-Fi: OK" : "Wi-Fi: --");
    st7796_draw_text(320, 6, 1, COL_ACC, COL_HDR, wifi);
    st7796_fill_rect(400, 2, 72, 20, COL_ERR);
    st7796_draw_text(418, 6, 1, COL_FG, COL_ERR, "OFF");

    /* Meta row */
    st7796_fill_rect(0, HDR_H, ST7796_W, META_H, COL_HDR);
    char meta[64];
    if (store && store->http_status) {
        snprintf(meta, sizeof(meta), "GET %u  %s",
                 (unsigned)store->http_status,
                 store->content_type[0] ? store->content_type : "-");
    } else {
        snprintf(meta, sizeof(meta), "webhook.site");
    }
    st7796_draw_text(8, HDR_H + 6, 1, COL_WARN, COL_HDR, meta);

    /* Body */
    char line[96];
    uint16_t top = store ? store->top_line : 0;
    uint16_t lines = store ? store->line_count : 0;
    for (uint16_t row = 0; row < VIEW_BODY_ROWS; row++) {
        int y = BODY_Y + 4 + (int)row * 8;
        uint16_t li = (uint16_t)(top + row);
        if (store && li < lines && response_store_get_line(store, li, line, sizeof(line))) {
            st7796_draw_text(8, y, 1, COL_FG, COL_BG, line);
        }
    }

    /* Status strip above buttons */
    const char *st = net ? http_net_status(net) : "";
    st7796_fill_rect(0, ST7796_H - FOOT_H, ST7796_W, 8, COL_HDR);
    st7796_draw_text(8, ST7796_H - FOOT_H, 1, COL_WARN, COL_HDR, st);

    draw_btn(BTN0_X, "PREV");
    draw_btn(BTN0_X + (BTN_W + BTN_GAP), "REFRESH");
    draw_btn(BTN0_X + 2 * (BTN_W + BTN_GAP),
             (net && (net->state == HTTP_NET_REQUESTING ||
                      net->state == HTTP_NET_RECEIVING))
                 ? "CANCEL"
                 : "NEXT");

    v->dirty = false;
}

view_hit_t view_hit(uint16_t x, uint16_t y) {
    if (x >= 400 && x < 472 && y < HDR_H) return VIEW_HIT_OFF;

    if (y < BTN_Y || y >= BTN_Y + BTN_H) return VIEW_HIT_NONE;
    int rel = (int)x - BTN0_X;
    if (rel < 0) return VIEW_HIT_NONE;
    int slot = rel / (BTN_W + BTN_GAP);
    int within = rel % (BTN_W + BTN_GAP);
    if (within >= BTN_W) return VIEW_HIT_NONE;
    if (slot == 0) return VIEW_HIT_PREV;
    if (slot == 1) return VIEW_HIT_REFRESH;
    if (slot == 2) return VIEW_HIT_CANCEL; /* CANCEL or NEXT — main decides */
    return VIEW_HIT_NONE;
}
