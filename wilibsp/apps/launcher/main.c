/*
 * Persistent flash/XIP home shell.
 * Center button loads apps/store_ram.uf2 from USB MSC and launches it as a
 * RAM-only image. Reset / EXIT in the RAM app returns here.
 *
 * Media: FreeWili BSP has USB FatFs (0:/), not an SD driver — place the UF2 at
 *   0:/apps/store_ram.uf2
 * (USB thumb drive), matching the AGENTS_FREEWILI_RAM_APP_LOADING architecture
 * with USB substituted for SD.
 */
#include "fw2.h"
#include "app_loader.h"
#include "platform/diag.h"
#include "platform/psram.h"
#include "usbhost/usb_store.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define COL_BG     0x0000
#define COL_FG     0xFFFF
#define COL_ACCENT 0x07E0
#define COL_DIM    0x8410
#define COL_BTN    0x001F
#define COL_ERR    0x00F8

#define BTN_W  280
#define BTN_H  72
#define BTN_X  ((ST7796_W - BTN_W) / 2)
#define BTN_Y  ((ST7796_H - BTN_H) / 2)

typedef enum {
    UI_IDLE = 0,
    UI_LOADING,
    UI_ERROR,
} ui_state_t;

static ui_state_t s_ui = UI_IDLE;
static bool s_launch_req;
static char s_status[80];
static bool s_dirty = true;

static const char *STORE_UF2_PATHS[] = {
    "0:/apps/store_ram.uf2",
    "0:/freewili-store/apps/store_ram.uf2",
    "0:/store_ram.uf2",
};

static bool point_inside(uint16_t x, uint16_t y,
                         uint16_t left, uint16_t top,
                         uint16_t width, uint16_t height) {
    return x >= left && x < (uint16_t)(left + width) &&
           y >= top && y < (uint16_t)(top + height);
}

static void set_status(const char *msg) {
    snprintf(s_status, sizeof(s_status), "%s", msg ? msg : "");
    s_dirty = true;
}

static void draw_ui(bool usb_ok) {
    if (!s_dirty) return;
    st7796_fill_screen(COL_BG);
    st7796_draw_text(12, 12, 2, COL_FG, COL_BG, "FREE-WILi");
    st7796_draw_text(12, 40, 1, COL_DIM, COL_BG, "Home launcher (flash resident)");

    st7796_fill_rect(BTN_X, BTN_Y, BTN_W, BTN_H, COL_BTN);
    st7796_draw_text(BTN_X + 36, BTN_Y + 28, 2, COL_FG, COL_BTN, "APP STORE");

    if (s_ui == UI_LOADING) {
        st7796_draw_text(12, ST7796_H - 48, 1, COL_ACCENT, COL_BG, "Loading RAM app…");
    } else if (s_ui == UI_ERROR) {
        st7796_draw_text(12, ST7796_H - 48, 1, COL_ERR, COL_BG, s_status);
    } else {
        st7796_draw_text(12, ST7796_H - 48, 1, COL_DIM, COL_BG,
                         usb_ok ? "USB ready — tap APP STORE"
                                : "Insert USB with apps/store_ram.uf2");
    }
    if (s_status[0] && s_ui != UI_ERROR) {
        st7796_draw_text(12, ST7796_H - 28, 1, COL_DIM, COL_BG, s_status);
    }
    s_dirty = false;
}

static void request_launch(void) {
    if (s_ui == UI_LOADING || s_launch_req) return;
    s_launch_req = true;
}

static void do_launch(void) {
    s_ui = UI_LOADING;
    s_dirty = true;
    draw_ui(true);
    set_status("Validating UF2…");
    s_dirty = true;
    draw_ui(true);

    app_loader_policy_t policy;
    app_loader_policy_default(&policy);

    app_loader_result_t rc = APP_LOADER_ERR_OPEN_FAILED;
    const char *used = NULL;
    for (unsigned i = 0; i < sizeof(STORE_UF2_PATHS) / sizeof(STORE_UF2_PATHS[0]); i++) {
        DIAG("launcher: try %s\n", STORE_UF2_PATHS[i]);
        rc = app_loader_validate_and_stage(STORE_UF2_PATHS[i], &policy);
        if (rc == APP_LOADER_OK) {
            used = STORE_UF2_PATHS[i];
            break;
        }
    }

    if (rc != APP_LOADER_OK) {
        char buf[80];
        snprintf(buf, sizeof(buf), "Fail: %s", app_loader_result_str(rc));
        set_status(buf);
        s_ui = UI_ERROR;
        s_dirty = true;
        DIAG("launcher: stage failed (%s)\n", app_loader_result_str(rc));
        return;
    }

    DIAG("launcher: launching %s (%u bytes)\n", used ? used : "?",
         (unsigned)app_loader_staged_bytes());
    set_status("Handoff…");
    s_dirty = true;
    draw_ui(true);
    /* Irreversible — does not return. */
    app_loader_launch_staged();
}

int main(void) {
    board_init();
    st7796_init();
    board_backlight_set(1);
    ft6336_init();

    size_t psz = psram_init();
    DIAG("launcher up: sys=%u kHz psram=%u\n",
         BOARD_SYS_CLOCK_KHZ, (unsigned)psz);

    set_status(psz ? "PSRAM OK" : "PSRAM missing");
    usb_store_init();

    bool touching = false;
    bool was_mounted = false;

    while (true) {
        usb_store_task();
        bool mounted = usb_store_mounted();
        if (mounted != was_mounted) {
            was_mounted = mounted;
            s_dirty = true;
            if (!mounted && s_ui == UI_ERROR) {
                s_ui = UI_IDLE;
                set_status("USB removed");
            }
            DIAG("launcher: USB %s\n", mounted ? "mounted" : "removed");
        }

        uint16_t tx = 0, ty = 0;
        if (ft6336_poll(&tx, &ty)) {
            if (!touching) {
                touching = true;
                if (s_ui != UI_LOADING &&
                    point_inside(tx, ty, BTN_X, BTN_Y, BTN_W, BTN_H)) {
                    if (!mounted) {
                        set_status("Need USB stick");
                        s_ui = UI_ERROR;
                    } else {
                        request_launch();
                    }
                } else if (s_ui == UI_ERROR) {
                    /* Tap anywhere to clear error. */
                    s_ui = UI_IDLE;
                    set_status(mounted ? "USB ready" : "Waiting USB");
                }
            }
        } else {
            touching = false;
        }

        if (s_launch_req) {
            s_launch_req = false;
            do_launch();
        }

        draw_ui(mounted);
        sleep_ms(16);
    }
}
