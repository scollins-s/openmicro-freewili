#include "usbdev/pio_usb_cdc.h"

/* TODO: Pico-PIO-USB device CDC on PIN_USB_DP/PIN_USB_DM (GPIO 42/43).
 * Faces the PC for future HID/CDC OpenMicro mode. Stub keeps the API seam. */

bool pio_usb_cdc_init(void) { return false; }
bool pio_usb_cdc_ready(void) { return false; }
size_t pio_usb_cdc_write(const uint8_t *data, size_t len) {
    (void)data;
    (void)len;
    return 0;
}
size_t pio_usb_cdc_read(uint8_t *buf, size_t cap) {
    (void)buf;
    (void)cap;
    return 0;
}
void pio_usb_cdc_task(void) {}
