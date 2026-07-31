#include "om_leds.h"
#include "fw2.h"
#include "hardware/pio.h"

static rgb_t state_rgb(om_agent_state_t s) {
    switch (s) {
        case OM_STATE_EXECUTING: return (rgb_t){ .r = 0, .g = 0, .b = 255 };
        case OM_STATE_WAITING:   return (rgb_t){ .r = 255, .g = 176, .b = 0 };
        case OM_STATE_COMPLETE:  return (rgb_t){ .r = 0, .g = 255, .b = 0 };
        case OM_STATE_ERROR:     return (rgb_t){ .r = 255, .g = 0, .b = 0 };
        default:                 return (rgb_t){ .r = 20, .g = 20, .b = 20 };
    }
}

void om_leds_init(void) {
    ws2812_init(pio1, 0, PIN_LED_DATA);
    ws2812_set_brightness(24); /* dimmer default — battery + glare */
    ws2812_clear();
    ws2812_show();
}

void om_leds_blank(void) {
    ws2812_clear();
    ws2812_show();
}

void om_leds_apply(const om_ui_state_t *st, uint8_t player_leds_mask) {
    if (!st) return;
    ws2812_clear();
    ws2812_set_pixel(0, state_rgb(st->state));
    rgb_t on = { .r = 0, .g = 80, .b = 200 };
    rgb_t foc = state_rgb(st->state);
    rgb_t off = { .r = 0, .g = 0, .b = 0 };
    uint8_t mask = player_leds_mask ? player_leds_mask : (uint8_t)(st->session_mask & 0x1f);
    for (int i = 0; i < 5; i++) {
        rgb_t c = off;
        if (mask & (1u << i)) c = (st->focus_index == i) ? foc : on;
        ws2812_set_pixel((uint)(i + 1), c);
    }
    /* Pixels 6..15 stay cleared — no fill of unused LEDs. */
    ws2812_show();
}
