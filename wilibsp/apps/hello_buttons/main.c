/* hello_buttons — smoke test for the provisional 14-button UART + haptic drivers.
 * Polls button frames; pulses haptic on OK press. Status on RTT + LED0. */
#include "fw2.h"
#include "platform/diag.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"

int main(void) {
    board_init();
    buttons_init();
    haptic_init();

    ws2812_init(pio1, 0, PIN_LED_DATA);
    ws2812_set_brightness(40);
    ws2812_clear();
    ws2812_show();

    DIAG("hello_buttons: polling UART1 @ 115200 (GPIO38/39), haptic GPIO46\n");

    absolute_time_t next_led = get_absolute_time();
    for (;;) {
        buttons_state_t st;
        buttons_poll(&st);
        if (st.valid && buttons_pressed(&st, BTN_OK)) {
            DIAG("OK pressed mask=0x%04x\n", st.mask);
            haptic_pulse_ms(40);
            rgb_t g = { .r = 0, .g = 255, .b = 0 };
            ws2812_set_pixel(0, g);
            ws2812_show();
        }
        if (absolute_time_diff_us(get_absolute_time(), next_led) <= 0) {
            if (st.valid && st.mask) {
                rgb_t b = { .r = 0, .g = 40, .b = 200 };
                ws2812_set_pixel(0, b);
            } else {
                rgb_t d = { .r = 8, .g = 8, .b = 8 };
                ws2812_set_pixel(0, d);
            }
            ws2812_show();
            next_led = make_timeout_time_ms(100);
        }
        sleep_ms(5);
    }
}
