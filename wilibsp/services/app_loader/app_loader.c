#include "app_loader.h"
#include "handoff.h"

#include "platform/diag.h"
#include "platform/psram.h"
#include "pico/stdlib.h"
#include "ff.h"
#include <string.h>

/*
 * PSRAM layout for staging (offsets from PSRAM_BASE):
 *   0x00400000  uf2_accum_t mirror
 *   0x00410000  app_loader_stage_rec_t[UF2_MAX_BLOCKS]
 *   0x00450000  packed payload bytes
 *   0x007F0000  handoff stack (see handoff.c)
 */
#define STAGE_ACC_OFF    0x00400000u
#define STAGE_RECS_OFF   0x00410000u
#define STAGE_DATA_OFF   0x00450000u
#define STAGE_DATA_SIZE  0x002A0000u

static bool s_staged;
static uf2_accum_t s_acc;
static app_loader_policy_t s_policy;
static uint32_t s_data_used;

void app_loader_policy_default(app_loader_policy_t *policy) {
    if (!policy) return;
    memset(policy, 0, sizeof(*policy));
    uf2_policy_default_sram(&policy->policy);
}

app_loader_result_t app_loader_from_uf2(uf2_result_t r) {
    switch (r) {
    case UF2_OK: return APP_LOADER_OK;
    case UF2_ERR_BAD_FAMILY: return APP_LOADER_ERR_WRONG_FAMILY;
    case UF2_ERR_BAD_ADDRESS: return APP_LOADER_ERR_ADDRESS_RANGE;
    case UF2_ERR_IMAGE_TOO_LARGE:
    case UF2_ERR_TOO_MANY_BLOCKS: return APP_LOADER_ERR_IMAGE_TOO_LARGE;
    case UF2_ERR_MISSING_BLOCK: return APP_LOADER_ERR_MISSING_BLOCK;
    case UF2_ERR_DUPLICATE_CONFLICT: return APP_LOADER_ERR_DUPLICATE_CONFLICT;
    default: return APP_LOADER_ERR_INVALID_UF2;
    }
}

const char *app_loader_result_str(app_loader_result_t r) {
    switch (r) {
    case APP_LOADER_OK: return "ok";
    case APP_LOADER_ERR_INVALID_ARGUMENT: return "invalid argument";
    case APP_LOADER_ERR_PSRAM: return "PSRAM init failed";
    case APP_LOADER_ERR_OPEN_FAILED: return "open failed";
    case APP_LOADER_ERR_READ_FAILED: return "read failed";
    case APP_LOADER_ERR_INVALID_UF2: return "invalid UF2";
    case APP_LOADER_ERR_WRONG_FAMILY: return "wrong family";
    case APP_LOADER_ERR_ADDRESS_RANGE: return "bad address range";
    case APP_LOADER_ERR_IMAGE_TOO_LARGE: return "image too large";
    case APP_LOADER_ERR_MISSING_BLOCK: return "missing block";
    case APP_LOADER_ERR_DUPLICATE_CONFLICT: return "duplicate conflict";
    case APP_LOADER_ERR_STAGE_MEMORY: return "stage memory full";
    case APP_LOADER_ERR_NOT_STAGED: return "nothing staged";
    case APP_LOADER_ERR_HANDOFF: return "handoff failed";
    default: return "unknown loader error";
    }
}

bool app_loader_is_staged(void) { return s_staged; }
uint32_t app_loader_staged_bytes(void) { return s_staged ? s_acc.total_payload : 0; }

void app_loader_abort(void) {
    s_staged = false;
    s_data_used = 0;
    memset(&s_acc, 0, sizeof(s_acc));
}

static app_loader_stage_rec_t *stage_recs(void) {
    return (app_loader_stage_rec_t *)(PSRAM_BASE + STAGE_RECS_OFF);
}

static uint8_t *stage_data(void) {
    return (uint8_t *)(PSRAM_BASE + STAGE_DATA_OFF);
}

static uf2_accum_t *stage_acc_mirror(void) {
    return (uf2_accum_t *)(PSRAM_BASE + STAGE_ACC_OFF);
}

app_loader_result_t app_loader_validate_and_stage(const char *path,
                                                  const app_loader_policy_t *policy) {
    if (!path || !path[0]) return APP_LOADER_ERR_INVALID_ARGUMENT;

    app_loader_abort();
    if (policy) s_policy = *policy;
    else app_loader_policy_default(&s_policy);

    size_t psz = psram_init();
    if (psz < (STAGE_DATA_OFF + 0x10000u)) {
        DIAG("app_loader: PSRAM missing/small (%u)\n", (unsigned)psz);
        return APP_LOADER_ERR_PSRAM;
    }

    FIL fil;
    if (f_open(&fil, path, FA_READ) != FR_OK) {
        DIAG("app_loader: open failed %s\n", path);
        return APP_LOADER_ERR_OPEN_FAILED;
    }

    uf2_accum_begin(&s_acc);
    app_loader_stage_rec_t *recs = stage_recs();
    uint8_t *data = stage_data();
    s_data_used = 0;

    uint8_t raw[UF2_BLOCK_SIZE];
    uf2_block_t blk;
    UINT br = 0;
    app_loader_result_t rc = APP_LOADER_OK;

    for (;;) {
        FRESULT fr = f_read(&fil, raw, UF2_BLOCK_SIZE, &br);
        if (fr != FR_OK) {
            rc = APP_LOADER_ERR_READ_FAILED;
            break;
        }
        if (br == 0) break;
        if (br != UF2_BLOCK_SIZE) {
            rc = APP_LOADER_ERR_INVALID_UF2;
            break;
        }

        uf2_result_t ur = uf2_parse_block(raw, &blk);
        if (ur != UF2_OK) {
            rc = app_loader_from_uf2(ur);
            DIAG("app_loader: parse %s\n", uf2_result_str(ur));
            break;
        }

        bool already = s_acc.seen[blk.block_no] != 0;
        ur = uf2_accum_add(&s_acc, &blk, &s_policy.policy);
        if (ur != UF2_OK) {
            rc = app_loader_from_uf2(ur);
            DIAG("app_loader: accum %s\n", uf2_result_str(ur));
            break;
        }
        if (already) continue;

        if (s_data_used + blk.payload_size > STAGE_DATA_SIZE) {
            rc = APP_LOADER_ERR_STAGE_MEMORY;
            break;
        }

        app_loader_stage_rec_t *rec = &recs[blk.block_no];
        rec->target_addr = blk.target_addr;
        rec->payload_size = (uint16_t)blk.payload_size;
        rec->reserved = 0;
        rec->data_off = s_data_used;
        memcpy(data + s_data_used, blk.data, blk.payload_size);
        s_data_used += blk.payload_size;
    }

    f_close(&fil);

    if (rc != APP_LOADER_OK) {
        app_loader_abort();
        return rc;
    }

    uf2_result_t ur = uf2_accum_finalize(&s_acc);
    if (ur != UF2_OK) {
        DIAG("app_loader: finalize %s\n", uf2_result_str(ur));
        app_loader_abort();
        return app_loader_from_uf2(ur);
    }

    /* Mirror metadata into PSRAM so final copy never depends on SRAM state. */
    *stage_acc_mirror() = s_acc;
    s_staged = true;
    DIAG("app_loader: staged %u bytes, %u blocks, %08x..%08x\n",
         (unsigned)s_acc.total_payload, (unsigned)s_acc.num_blocks,
         (unsigned)s_acc.min_addr, (unsigned)s_acc.max_addr_exclusive);
    return APP_LOADER_OK;
}

_Noreturn void app_loader_launch_staged(void) {
    if (!s_staged) {
        DIAG("app_loader: launch with nothing staged\n");
        for (;;) tight_loop_contents();
    }
    /* Prefer PSRAM mirrors — SRAM will be overwritten. */
    const uf2_accum_t *acc = stage_acc_mirror();
    const app_loader_stage_rec_t *recs = stage_recs();
    const uint8_t *data = stage_data();
    app_loader_final_copy_and_launch(acc, recs, data);
}
