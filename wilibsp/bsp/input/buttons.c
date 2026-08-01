#include "input/buttons.h"
#include "platform/board.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define BTN_UART           uart1
#define BTN_BAUD           115200
#define BTN_FRAME_SYNC     0xAAu

static uint16_t g_prev_mask;
static bool g_have_prev;
static bool g_valid;
static int g_sync_state; /* 0=wait AA, 1=lo, 2=hi, 3=xor */
static uint8_t g_lo, g_hi;

void buttons_init(void) {
    uart_init(BTN_UART, BTN_BAUD);
    gpio_set_function(PIN_BTN_TX, GPIO_FUNC_UART);
    gpio_set_function(PIN_BTN_RX, GPIO_FUNC_UART);
    uart_set_hw_flow(BTN_UART, false, false);
    uart_set_format(BTN_UART, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(BTN_UART, true);
    g_prev_mask = 0;
    g_have_prev = false;
    g_valid = false;
    g_sync_state = 0;
}

void buttons_poll(buttons_state_t *out) {
    if (!out) return;
    uint16_t mask = g_have_prev ? g_prev_mask : 0;

    while (uart_is_readable(BTN_UART)) {
        uint8_t b = (uint8_t)uart_getc(BTN_UART);
        switch (g_sync_state) {
            case 0:
                if (b == BTN_FRAME_SYNC) g_sync_state = 1;
                break;
            case 1:
                g_lo = b;
                g_sync_state = 2;
                break;
            case 2:
                g_hi = b;
                g_sync_state = 3;
                break;
            case 3: {
                uint8_t expect = (uint8_t)(BTN_FRAME_SYNC ^ g_lo ^ g_hi);
                if (b == expect) {
                    mask = (uint16_t)g_lo | ((uint16_t)g_hi << 8);
                    mask &= 0x3FFF; /* 14 bits */
                    g_valid = true;
                }
                g_sync_state = 0;
                break;
            }
            default:
                g_sync_state = 0;
                break;
        }
    }

    out->mask = mask;
    out->valid = g_valid;
    if (g_have_prev) {
        out->pressed = (uint16_t)(mask & ~g_prev_mask);
        out->released = (uint16_t)(g_prev_mask & ~mask);
    } else {
        out->pressed = 0;
        out->released = 0;
        g_have_prev = true;
    }
    g_prev_mask = mask;
}
