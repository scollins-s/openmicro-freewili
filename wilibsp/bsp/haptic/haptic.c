#include "haptic/haptic.h"
#include "platform/board.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

void haptic_init(void) {
    gpio_init(PIN_HAPTIC);
    gpio_set_dir(PIN_HAPTIC, GPIO_OUT);
    gpio_put(PIN_HAPTIC, 0);
}

void haptic_off(void) {
    gpio_put(PIN_HAPTIC, 0);
}

void haptic_pulse_ms(uint32_t ms) {
    if (ms == 0) {
        haptic_off();
        return;
    }
    gpio_put(PIN_HAPTIC, 1);
    sleep_ms(ms);
    gpio_put(PIN_HAPTIC, 0);
}
