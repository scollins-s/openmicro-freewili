#include "voice_ui.h"
#include "fw2.h"
#include <stdio.h>
#include <string.h>

/* Layout for 480x320 */
#define REC_X 140
#define REC_Y 100
#define REC_W 200
#define REC_H 80

void voice_ui_init(voice_ui_state_t *st) {
    memset(st, 0, sizeof(*st));
    st->mode = VOICE_UI_NEED_STICK;
    snprintf(st->status, sizeof(st->status), "Insert USB stick");
    st->dirty = true;
}

void voice_ui_set_status(voice_ui_state_t *st, voice_ui_mode_t mode, const char *msg) {
    st->mode = mode;
    if (msg) {
        snprintf(st->status, sizeof(st->status), "%s", msg);
    }
    st->dirty = true;
}

void voice_ui_draw(const voice_ui_state_t *st) {
    st7796_fill_screen(0x0000);
    st7796_draw_text(12, 12, 2, 0xFFFF, 0x0000, "OPENMICRO VOICE");
    st7796_draw_text(12, 40, 1, 0xC618, 0x0000, "PDM -> WAV on USB (phase 1)");

    uint16_t rec_bg = 0x00F8; /* red (byte-swapped) */
    if (st->mode == VOICE_UI_RECORDING) rec_bg = 0xE0FF; /* amber */
    else if (st->mode == VOICE_UI_OK) rec_bg = 0xE007; /* green */
    else if (st->mode == VOICE_UI_ERR) rec_bg = 0x00F8;
    st7796_fill_rect(REC_X, REC_Y, REC_W, REC_H, rec_bg);
    st7796_draw_text(REC_X + 70, REC_Y + 30, 2, 0xFFFF, rec_bg, "REC");

    st7796_draw_text(12, 200, 1, 0xFFFF, 0x0000, st->status);

    /* Simple RMS bar (0..200 px from rms/80) */
    unsigned bar = st->rms / 80u;
    if (bar > 200u) bar = 200u;
    st7796_fill_rect(12, 230, 200, 16, 0x2104);
    if (bar) st7796_fill_rect(12, 230, (uint16_t)bar, 16, 0xFF07); /* cyan */

    char line[40];
    snprintf(line, sizeof(line), "rms=%u clip#=%u", st->rms, st->clip_n);
    st7796_draw_text(12, 256, 1, 0xC618, 0x0000, line);
    st7796_draw_text(12, 280, 1, 0x8410, 0x0000, "Tap REC: 3s mono 16kHz wav");
}

voice_hit_t voice_ui_hit(uint16_t x, uint16_t y) {
    if (x >= REC_X && x < REC_X + REC_W && y >= REC_Y && y < REC_Y + REC_H) {
        return VOICE_HIT_REC;
    }
    return VOICE_HIT_NONE;
}
