/* USB FatFs → PSRAM stage → ROM RAM-image handoff for no_flash UF2 apps. */
#ifndef APP_LOADER_H
#define APP_LOADER_H

#include "uf2_reader.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_LOADER_OK = 0,
    APP_LOADER_ERR_INVALID_ARGUMENT,
    APP_LOADER_ERR_PSRAM,
    APP_LOADER_ERR_OPEN_FAILED,
    APP_LOADER_ERR_READ_FAILED,
    APP_LOADER_ERR_INVALID_UF2,
    APP_LOADER_ERR_WRONG_FAMILY,
    APP_LOADER_ERR_ADDRESS_RANGE,
    APP_LOADER_ERR_IMAGE_TOO_LARGE,
    APP_LOADER_ERR_MISSING_BLOCK,
    APP_LOADER_ERR_DUPLICATE_CONFLICT,
    APP_LOADER_ERR_STAGE_MEMORY,
    APP_LOADER_ERR_NOT_STAGED,
    APP_LOADER_ERR_HANDOFF,
} app_loader_result_t;

typedef struct {
    uf2_policy_t policy;
} app_loader_policy_t;

void app_loader_policy_default(app_loader_policy_t *policy);

/* Read + validate UF2 from FatFs path; stage payloads into PSRAM.
 * Safe to call while UI is still running. Does not hand off. */
app_loader_result_t app_loader_validate_and_stage(const char *path,
                                                  const app_loader_policy_t *policy);

/* Irreversible: quiesce peripherals, copy staged image into SRAM, rom_reboot. */
_Noreturn void app_loader_launch_staged(void);

void app_loader_abort(void);

bool app_loader_is_staged(void);
uint32_t app_loader_staged_bytes(void);
const char *app_loader_result_str(app_loader_result_t r);

/* Map uf2_result_t into app_loader_result_t. */
app_loader_result_t app_loader_from_uf2(uf2_result_t r);

#ifdef __cplusplus
}
#endif

#endif
