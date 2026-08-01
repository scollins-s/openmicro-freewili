// bsp/haptic/haptic.h — GPIO digital pulse driver for the FreeWili2 haptic motor.
// PIN_HAPTIC = 46. PWM/envelope can come later; this is a simple on/off pulse.
#ifndef HAPTIC_H
#define HAPTIC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void haptic_init(void);
void haptic_pulse_ms(uint32_t ms);
void haptic_off(void);

#ifdef __cplusplus
}
#endif
#endif
