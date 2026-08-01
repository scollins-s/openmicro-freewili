#ifndef VOICE_UI_H
#define VOICE_UI_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    VOICE_UI_IDLE = 0,
    VOICE_UI_NEED_STICK,
    VOICE_UI_RECORDING,
    VOICE_UI_SAVING,
    VOICE_UI_OK,
    VOICE_UI_ERR,
} voice_ui_mode_t;

typedef struct {
    voice_ui_mode_t mode;
    unsigned rms;
    unsigned clip_n;
    char status[48];
    bool dirty;
} voice_ui_state_t;

typedef enum {
    VOICE_HIT_NONE = 0,
    VOICE_HIT_REC,
} voice_hit_t;

void voice_ui_init(voice_ui_state_t *st);
void voice_ui_draw(const voice_ui_state_t *st);
voice_hit_t voice_ui_hit(uint16_t x, uint16_t y);
void voice_ui_set_status(voice_ui_state_t *st, voice_ui_mode_t mode, const char *msg);

#endif
