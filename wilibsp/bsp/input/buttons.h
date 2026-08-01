// bsp/input/buttons.h — 14-button serial coprocessor on UART1 (GPIO38 TX / GPIO39 RX).
//
// PROVISIONAL PROTOCOL (not yet confirmed against production firmware):
//   Frame @ 115200 8N1:  0xAA  <mask_lo>  <mask_hi>  <xor>
//   where xor = 0xAA ^ mask_lo ^ mask_hi
//   mask bits (lsb first):
//     0 Up  1 Down  2 Left  3 Right  4 Center  5 Home  6 OK  7 Cancel
//     8 Page  9 Grey  10 Yellow  11 Green  12 Blue  13 Red
//
// UART0 is reserved for FwGUI/OneWili — this driver uses uart1 exclusively.
#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    BTN_UP = 0,
    BTN_DOWN,
    BTN_LEFT,
    BTN_RIGHT,
    BTN_CENTER,
    BTN_HOME,
    BTN_OK,
    BTN_CANCEL,
    BTN_PAGE,
    BTN_GREY,
    BTN_YELLOW,
    BTN_GREEN,
    BTN_BLUE,
    BTN_RED,
    BTN_COUNT = 14,
};

typedef struct {
    uint16_t mask;       /* current pressed bits */
    uint16_t pressed;    /* edges: 0→1 since last poll */
    uint16_t released;   /* edges: 1→0 since last poll */
    bool     valid;      /* true if at least one good frame seen */
} buttons_state_t;

void buttons_init(void);
void buttons_poll(buttons_state_t *out);

static inline bool buttons_down(const buttons_state_t *s, int id) {
    return s && id >= 0 && id < BTN_COUNT && (s->mask & (1u << id)) != 0;
}
static inline bool buttons_pressed(const buttons_state_t *s, int id) {
    return s && id >= 0 && id < BTN_COUNT && (s->pressed & (1u << id)) != 0;
}
static inline bool buttons_released(const buttons_state_t *s, int id) {
    return s && id >= 0 && id < BTN_COUNT && (s->released & (1u << id)) != 0;
}

#ifdef __cplusplus
}
#endif
#endif
