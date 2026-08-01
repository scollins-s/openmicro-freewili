/* openmicro_voice — PDM ×4 capture → mono WAV on USB MSC (v2 phase 1 proof). */
#include "fw2.h"
#include "platform/diag.h"
#include "platform/psram.h"
#include "platform/ioexp.h"
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "ff.h"
#include "voice_ui.h"
#include "voice_wav.h"
#include <stdio.h>
#include <string.h>

#define REC_SECONDS     3u
#define REC_FRAMES      (VOICE_WAV_RATE_HZ * REC_SECONDS)
#define BLOCK_FRAMES    1600u   /* 100 ms @ 16 kHz */
#define PSRAM_PCM_OFF   0x0u    /* first region of PSRAM for PCM */

static int16_t s_block[PDM_NUM_MICS][BLOCK_FRAMES];
static bool s_pdm_ready;
static unsigned s_clip_n;

static void leds_idle(void) {
    ws2812_clear();
    ws2812_set_pixel(0, (rgb_t){ .r = 20, .g = 20, .b = 20 });
    ws2812_show();
}

static void leds_recording(void) {
    ws2812_clear();
    ws2812_set_pixel(0, (rgb_t){ .r = 255, .g = 160, .b = 0 });
    ws2812_show();
}

static void leds_ok(void) {
    ws2812_clear();
    ws2812_set_pixel(0, (rgb_t){ .r = 0, .g = 255, .b = 0 });
    ws2812_show();
}

static void leds_err(void) {
    ws2812_clear();
    ws2812_set_pixel(0, (rgb_t){ .r = 255, .g = 0, .b = 0 });
    ws2812_show();
}

static uint32_t rms_of(const int16_t *buf, unsigned n) {
    uint64_t sumsq = 0;
    for (unsigned i = 0; i < n; i++) {
        int32_t v = buf[i];
        sumsq += (uint64_t)(v * v);
    }
    /* Integer sqrt of mean square — coarse but fine for a bar. */
    uint32_t mean = (uint32_t)(sumsq / (n ? n : 1u));
    uint32_t r = 0, b = 1u << 30;
    while (b > mean) b >>= 2;
    while (b) {
        if (mean >= r + b) {
            mean -= r + b;
            r = (r >> 1) + b;
        } else {
            r >>= 1;
        }
        b >>= 2;
    }
    return r;
}

static bool ensure_pdm(void) {
    if (s_pdm_ready) {
        ioexp_mic_pwr(true);
        sleep_ms(30);
        return true;
    }
    pdm_capture_init();
    s_pdm_ready = true;
    return true;
}

static bool ensure_dir(void) {
    FRESULT fr = f_mkdir("0:/openmicro");
    if (fr == FR_OK || fr == FR_EXIST) return true;
    DIAG("openmicro_voice: mkdir fr=%d\n", (int)fr);
    return false;
}

static unsigned next_clip_index(void) {
    /* Scan for next free voice_NNN.wav (0..999). */
    char path[40];
    for (unsigned i = 0; i < 1000u; i++) {
        snprintf(path, sizeof(path), "0:/openmicro/voice_%03u.wav", i);
        FILINFO fi;
        if (f_stat(path, &fi) != FR_OK) return i;
    }
    return 999;
}

static void do_record(voice_ui_state_t *ui) {
    if (!usb_store_mounted()) {
        voice_ui_set_status(ui, VOICE_UI_NEED_STICK, "Insert USB stick");
        leds_err();
        return;
    }
    if (!ensure_dir()) {
        voice_ui_set_status(ui, VOICE_UI_ERR, "mkdir failed");
        leds_err();
        return;
    }

    size_t psz = psram_init();
    if (psz < REC_FRAMES * sizeof(int16_t)) {
        voice_ui_set_status(ui, VOICE_UI_ERR, "PSRAM too small");
        leds_err();
        DIAG("openmicro_voice: PSRAM %u too small\n", (unsigned)psz);
        return;
    }
    int16_t *pcm = (int16_t *)(PSRAM_BASE + PSRAM_PCM_OFF);

    if (!ensure_pdm()) {
        voice_ui_set_status(ui, VOICE_UI_ERR, "PDM init failed");
        leds_err();
        return;
    }

    voice_ui_set_status(ui, VOICE_UI_RECORDING, "Recording 3s...");
    leds_recording();
    voice_ui_draw(ui);
    ui->dirty = false;
    DIAG("openmicro_voice: recording %u frames...\n", (unsigned)REC_FRAMES);

    unsigned got = 0;
    unsigned peak_rms = 0;
    while (got < REC_FRAMES) {
        unsigned want = REC_FRAMES - got;
        if (want > BLOCK_FRAMES) want = BLOCK_FRAMES;
        int16_t *dst[PDM_NUM_MICS] = {
            s_block[0], s_block[1], s_block[2], s_block[3],
        };
        pdm_capture_block(dst, want);
        dcblock_inplace(s_block[MIC_A], want);
        memcpy(pcm + got, s_block[MIC_A], want * sizeof(int16_t));
        uint32_t rms = rms_of(s_block[MIC_A], want);
        if (rms > peak_rms) peak_rms = rms;
        ui->rms = rms;
        got += want;

        /* Light UI refresh every ~300 ms without full redraw cost every block */
        if ((got / BLOCK_FRAMES) % 3u == 0u) {
            char msg[40];
            snprintf(msg, sizeof(msg), "Recording %u/%us", got / VOICE_WAV_RATE_HZ, REC_SECONDS);
            voice_ui_set_status(ui, VOICE_UI_RECORDING, msg);
            voice_ui_draw(ui);
            ui->dirty = false;
            leds_recording();
        }
        usb_store_task();
    }

    ioexp_mic_pwr(false);

    unsigned idx = next_clip_index();
    s_clip_n = idx + 1;
    char path[40];
    snprintf(path, sizeof(path), "0:/openmicro/voice_%03u.wav", idx);

    voice_ui_set_status(ui, VOICE_UI_SAVING, "Saving WAV...");
    voice_ui_draw(ui);
    ui->dirty = false;

    if (!voice_wav_write(path, pcm, REC_FRAMES)) {
        voice_ui_set_status(ui, VOICE_UI_ERR, "WAV write failed");
        leds_err();
        return;
    }

    char ok[48];
    snprintf(ok, sizeof(ok), "OK %s rms=%u", path + 3, peak_rms); /* strip 0: */
    ui->rms = peak_rms;
    ui->clip_n = s_clip_n;
    voice_ui_set_status(ui, VOICE_UI_OK, ok);
    leds_ok();
    DIAG("openmicro_voice: done %s peak_rms=%u\n", path, peak_rms);
}

int main(void) {
    board_init();
    st7796_init();
    board_backlight_set(1);
    ft6336_init();

    /* Claim SM0 for LEDs before PDM claims an unused SM on pio1. */
    pio_sm_claim(pio1, 0);
    ws2812_init(pio1, 0, PIN_LED_DATA);
    ws2812_set_brightness(32);
    leds_idle();

    usb_store_init();

    voice_ui_state_t ui;
    voice_ui_init(&ui);
    voice_ui_draw(&ui);
    ui.dirty = false;

    DIAG("\n=== openmicro_voice: PDM -> WAV on USB MSC ===\n");
    DIAG("Insert FAT32 stick, tap REC for 3s capture (mic A).\n");

    bool was_mounted = false;
    uint16_t tx = 0, ty = 0;
    bool touching = false;

    while (1) {
        usb_store_task();
        bool m = usb_store_mounted();
        if (m != was_mounted) {
            was_mounted = m;
            if (m) {
                voice_ui_set_status(&ui, VOICE_UI_IDLE, "Ready — tap REC");
                leds_idle();
            } else {
                voice_ui_set_status(&ui, VOICE_UI_NEED_STICK, "Insert USB stick");
                leds_idle();
            }
        }

        uint16_t x, y;
        bool down = ft6336_poll(&x, &y);
        if (down && !touching) {
            touching = true;
            tx = x;
            ty = y;
            if (ui.mode != VOICE_UI_RECORDING && ui.mode != VOICE_UI_SAVING) {
                if (voice_ui_hit(tx, ty) == VOICE_HIT_REC) {
                    do_record(&ui);
                }
            }
        } else if (!down) {
            touching = false;
        }

        if (ui.dirty) {
            voice_ui_draw(&ui);
            ui.dirty = false;
        }

        sleep_ms(16);
    }
}
