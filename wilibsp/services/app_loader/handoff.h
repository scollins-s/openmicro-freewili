#ifndef APP_LOADER_HANDOFF_H
#define APP_LOADER_HANDOFF_H

#include "uf2_reader.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Packed payload records live in PSRAM after staging. */
typedef struct {
    uint32_t target_addr;
    uint16_t payload_size;
    uint16_t reserved;
    uint32_t data_off; /* offset from PSRAM stage data base */
} app_loader_stage_rec_t;

void app_loader_quiesce_system(void);

_Noreturn void app_loader_final_copy_and_launch(const uf2_accum_t *acc,
                                               const app_loader_stage_rec_t *recs,
                                               const uint8_t *data_base);

#ifdef __cplusplus
}
#endif

#endif
