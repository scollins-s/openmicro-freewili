#ifndef OM_LEDS_H
#define OM_LEDS_H

#include <stdint.h>
#include "om_ui.h"

/* Map OpenMicro lightbar + playerLeds onto 16 WS2812:
 * pixel 0 = lightbar/status, pixels 1..5 = session slots. */

void om_leds_init(void);
void om_leds_apply(const om_ui_state_t *st, uint8_t player_leds_mask);
/** Clear the strip (standby / power save). */
void om_leds_blank(void);

#endif
