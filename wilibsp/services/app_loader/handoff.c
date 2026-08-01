#include "handoff.h"

#include "fw2.h"
#include "platform/psram.h"
#include "pico/bootrom.h"
#include "boot/picoboot_constants.h"
#include "pico/stdlib.h"
#include "hardware/irq.h"
#include "hardware/sync.h"
#include <string.h>

/* Handoff stack in high PSRAM — clear of stage band at 4–6 MB. */
#define HANDOFF_STACK_BYTES  4096u
#define HANDOFF_STACK_OFF    0x007F0000u

typedef struct {
    const uf2_accum_t *acc;
    const app_loader_stage_rec_t *recs;
    const uint8_t *data_base;
} handoff_args_t;

static handoff_args_t s_args;

void app_loader_quiesce_system(void) {
    absolute_time_t t0 = get_absolute_time();
    while (st7796_flush_busy()) {
        if (absolute_time_diff_us(t0, get_absolute_time()) > 500000) break;
        tight_loop_contents();
    }
    board_backlight_set(0);
    irq_set_mask_enabled(0xffffffffu, false);
    __dmb();
}

/* Non-static so the asm branch keeps a stable symbol under -flto. */
__attribute__((noinline, noreturn, used))
void app_loader_final_copy_body_impl(void) {
    const uf2_accum_t *acc = s_args.acc;
    const app_loader_stage_rec_t *recs = s_args.recs;
    const uint8_t *data_base = s_args.data_base;

    for (uint32_t i = 0; i < acc->num_blocks; i++) {
        const app_loader_stage_rec_t *r = &recs[i];
        uint8_t *dst = (uint8_t *)(uintptr_t)r->target_addr;
        const uint8_t *src = data_base + r->data_off;
        memcpy(dst, src, r->payload_size);
    }
    __dmb();
    __dsb();

    uint32_t region_base = acc->min_addr & ~0x3u;
    uint32_t region_end = (acc->max_addr_exclusive + 3u) & ~0x3u;
    uint32_t region_size = region_end - region_base;

    uint32_t flags = REBOOT2_FLAG_REBOOT_TYPE_RAM_IMAGE |
                     REBOOT2_FLAG_REBOOT_TO_ARM |
                     REBOOT2_FLAG_NO_RETURN_ON_SUCCESS;
    (void)rom_reboot(flags, 1, region_base, region_size);

    for (;;) {
        tight_loop_contents();
    }
}

_Noreturn void app_loader_final_copy_and_launch(const uf2_accum_t *acc,
                                               const app_loader_stage_rec_t *recs,
                                               const uint8_t *data_base) {
    s_args.acc = acc;
    s_args.recs = recs;
    s_args.data_base = data_base;
    app_loader_quiesce_system();

    uintptr_t stack_top =
        (uintptr_t)(PSRAM_BASE + HANDOFF_STACK_OFF + HANDOFF_STACK_BYTES);
    stack_top &= ~0x7u;

    /* Switch SP into PSRAM, then never return through the old stack. */
    __asm volatile(
        "mov sp, %0\n"
        "b app_loader_final_copy_body_impl\n"
        :
        : "r"(stack_top)
        : "memory");
    __builtin_unreachable();
}
