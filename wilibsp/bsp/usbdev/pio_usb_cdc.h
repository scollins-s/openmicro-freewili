// bsp/usbdev/pio_usb_cdc.h — seam for future Pico-PIO-USB device CDC on GPIO42/43
// (faces the PC). Not implemented yet; stubs return false / not-ready so callers
// can compile against the API. See docs/hardware/catalog.md (PIO-USB TODO).
#ifndef PIO_USB_CDC_H
#define PIO_USB_CDC_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Bring up PIO-USB device CDC. Always returns false until implemented. */
bool pio_usb_cdc_init(void);

/** True when a host has enumerated the CDC interface. Always false for now. */
bool pio_usb_cdc_ready(void);

/** Non-blocking write; returns bytes accepted (0 today). */
size_t pio_usb_cdc_write(const uint8_t *data, size_t len);

/** Non-blocking read; returns bytes read (0 today). */
size_t pio_usb_cdc_read(uint8_t *buf, size_t cap);

/** Service IRQ/poll loop — no-op stub. */
void pio_usb_cdc_task(void);

#ifdef __cplusplus
}
#endif
#endif
