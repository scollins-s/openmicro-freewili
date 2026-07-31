#include "om_ui.h"
#include "fw2.h"
#include <stdio.h>
#include <string.h>

/* RGB565 byte-swapped wire colors (same convention as hello_display). */
#define COL_BG     0x0000
#define COL_STRIP  0x1082
#define COL_WHITE  0xFFFF
#define COL_DIM    0xEF7B
#define COL_ACCENT 0x1F00 /* green-ish swapped */
#define COL_RED    0x00F8
#define COL_BLUE   0x1F00
#define COL_AMBER  0xE0FF
#define COL_CYAN   0xFF07
#define COL_BTN    0x4208
#define COL_BTN2   0x2945

static uint16_t state_color(om_agent_state_t s) {
    switch (s) {
        case OM_STATE_EXECUTING: return COL_BLUE;
        case OM_STATE_WAITING:   return COL_AMBER;
        case OM_STATE_COMPLETE:  return COL_ACCENT;
        case OM_STATE_ERROR:     return COL_RED;
        default:                 return COL_DIM;
    }
}

static const char *state_name(om_agent_state_t s) {
    switch (s) {
        case OM_STATE_EXECUTING: return "EXEC";
        case OM_STATE_WAITING:   return "WAIT";
        case OM_STATE_COMPLETE:  return "DONE";
        case OM_STATE_ERROR:     return "ERR";
        default:                 return "IDLE";
    }
}

/* Layout: 480x320
 *  status strip y0-36
 *  primaries: Accept|Reject ; Voice|Model|New
 *  workflow row y178-218
 *  session chips y226-258
 *  chrome: thinking / layer / dpad y266-312
 */

typedef struct { int x, y, w, h; } rect_t;

static const rect_t R_ACCEPT = { 12,  44, 220, 58 };
static const rect_t R_REJECT = { 248, 44, 220, 58 };
static const rect_t R_VOICE  = { 12,  110, 148, 58 };
static const rect_t R_MODEL  = { 166, 110, 148, 58 };
static const rect_t R_NEW    = { 320, 110, 148, 58 };

static const rect_t R_WF[4] = {
    { 12, 178, 110, 36 },
    { 130,178, 110, 36 },
    { 248,178, 110, 36 },
    { 366,178, 102, 36 },
};

static const rect_t R_SESS[5] = {
    { 12,  226, 84, 28 },
    { 104, 226, 84, 28 },
    { 196, 226, 84, 28 },
    { 288, 226, 84, 28 },
    { 380, 226, 88, 28 },
};

static const rect_t R_THINK_M = { 12,  270, 48, 36 };
static const rect_t R_THINK_P = { 68,  270, 48, 36 };
static const rect_t R_LAYER   = { 128, 270, 64, 36 };
static const rect_t R_UP      = { 280, 266, 40, 28 };
static const rect_t R_LEFT    = { 236, 294, 40, 24 };
static const rect_t R_DOWN    = { 280, 294, 40, 24 };
static const rect_t R_RIGHT   = { 324, 294, 40, 24 };

static bool inside(const rect_t *r, uint16_t x, uint16_t y) {
    return (int)x >= r->x && (int)x < r->x + r->w &&
           (int)y >= r->y && (int)y < r->y + r->h;
}

static void draw_btn(const rect_t *r, uint16_t fill, const char *label) {
    st7796_fill_rect(r->x, r->y, r->w, r->h, fill);
    int tw = (int)strlen(label) * 6 * 2;
    int tx = r->x + (r->w - tw) / 2;
    int ty = r->y + (r->h - 16) / 2;
    if (tx < r->x + 4) tx = r->x + 4;
    st7796_draw_text(tx, ty, 2, COL_WHITE, fill, label);
}

void om_ui_init(om_ui_state_t *st) {
    memset(st, 0, sizeof(*st));
    st->state = OM_STATE_IDLE;
    st->session_mask = 0x01;
    st->dirty = true;
}

void om_ui_draw(const om_ui_state_t *st) {
    st7796_fill_screen(COL_BG);

    /* Status strip */
    uint16_t sc = state_color(st->state);
    st7796_fill_rect(0, 0, ST7796_W, 36, sc);
    char line[48];
    snprintf(line, sizeof line, "OM %s  L%d  %s",
             state_name(st->state), st->layer,
             st->link_up ? "LINK" : "DEMO");
    st7796_draw_text(8, 10, 2, COL_WHITE, sc, line);

    draw_btn(&R_ACCEPT, 0xE007 /* dark green swap */, "ACCEPT");
    draw_btn(&R_REJECT, COL_RED, "REJECT");
    draw_btn(&R_VOICE,  COL_BTN2, "VOICE");
    draw_btn(&R_MODEL,  COL_CYAN, "MODEL");
    draw_btn(&R_NEW,    COL_BTN, "NEW");

    static const char *wf[] = { "REVIEW", "DEBUG", "REFACTOR", "TESTS" };
    for (int i = 0; i < 4; i++) draw_btn(&R_WF[i], COL_BTN, wf[i]);

    for (int i = 0; i < 5; i++) {
        bool on = (st->session_mask & (1 << i)) != 0;
        bool foc = (st->focus_index == i);
        uint16_t c = foc ? sc : (on ? COL_BTN2 : COL_BTN);
        char lab[4];
        snprintf(lab, sizeof lab, "S%d", i + 1);
        draw_btn(&R_SESS[i], c, lab);
    }

    draw_btn(&R_THINK_M, COL_BTN, "-");
    draw_btn(&R_THINK_P, COL_BTN, "+");
    snprintf(line, sizeof line, "T%d", st->thinking);
    draw_btn(&R_LAYER, COL_BTN2, line);
    /* overwrite layer rect label with layer number after thinking row */
    snprintf(line, sizeof line, "L%d", st->layer);
    st7796_fill_rect(R_LAYER.x, R_LAYER.y, R_LAYER.w, R_LAYER.h, COL_BTN2);
    st7796_draw_text(R_LAYER.x + 16, R_LAYER.y + 10, 2, COL_WHITE, COL_BTN2, line);

    /* thinking value between -/+ */
    snprintf(line, sizeof line, "%d", st->thinking);
    st7796_draw_text(48, 248, 1, COL_DIM, COL_BG, "THINK");

    draw_btn(&R_UP, COL_BTN, "^");
    draw_btn(&R_LEFT, COL_BTN, "<");
    draw_btn(&R_DOWN, COL_BTN, "v");
    draw_btn(&R_RIGHT, COL_BTN, ">");
}

om_hit_t om_ui_hit(uint16_t x, uint16_t y) {
    if (inside(&R_ACCEPT, x, y)) return OM_HIT_ACCEPT;
    if (inside(&R_REJECT, x, y)) return OM_HIT_REJECT;
    if (inside(&R_VOICE, x, y))  return OM_HIT_VOICE;
    if (inside(&R_MODEL, x, y))  return OM_HIT_MODEL;
    if (inside(&R_NEW, x, y))    return OM_HIT_NEW;
    if (inside(&R_WF[0], x, y))  return OM_HIT_WF_REVIEW;
    if (inside(&R_WF[1], x, y))  return OM_HIT_WF_DEBUG;
    if (inside(&R_WF[2], x, y))  return OM_HIT_WF_REFACTOR;
    if (inside(&R_WF[3], x, y))  return OM_HIT_WF_TESTS;
    for (int i = 0; i < 5; i++) {
        if (inside(&R_SESS[i], x, y))
            return (om_hit_t)(OM_HIT_SESSION0 + i);
    }
    if (inside(&R_THINK_M, x, y)) return OM_HIT_THINK_MINUS;
    if (inside(&R_THINK_P, x, y)) return OM_HIT_THINK_PLUS;
    if (inside(&R_LAYER, x, y))   return OM_HIT_LAYER;
    if (inside(&R_UP, x, y))      return OM_HIT_DPAD_UP;
    if (inside(&R_DOWN, x, y))    return OM_HIT_DPAD_DOWN;
    if (inside(&R_LEFT, x, y))    return OM_HIT_DPAD_LEFT;
    if (inside(&R_RIGHT, x, y))   return OM_HIT_DPAD_RIGHT;
    return OM_HIT_NONE;
}
