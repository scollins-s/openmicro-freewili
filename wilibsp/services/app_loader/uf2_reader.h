/* UF2 block parse + SRAM-target validation (host-testable, no Pico SDK). */
#ifndef UF2_READER_H
#define UF2_READER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define UF2_MAGIC_START0  0x0A324655u
#define UF2_MAGIC_START1  0x9E5D5157u
#define UF2_MAGIC_END     0x0AB16F30u
#define UF2_FLAG_FAMILY   0x00002000u
#define UF2_BLOCK_SIZE    512u
#define UF2_PAYLOAD_MAX   476u

/* RP2350 Absolute / Arm Secure family IDs commonly emitted by Pico SDK. */
#define UF2_FAMILY_RP2350_ARM_S    0xE48BFF59u
#define UF2_FAMILY_RP2350_ABSOLUTE 0xE48BFF57u

/* Enough for a full 512 KiB SRAM image at 256 B useful payload average. */
#define UF2_MAX_BLOCKS    2048u

typedef struct __attribute__((packed)) {
    uint32_t magic_start0;
    uint32_t magic_start1;
    uint32_t flags;
    uint32_t target_addr;
    uint32_t payload_size;
    uint32_t block_no;
    uint32_t num_blocks;
    uint32_t family_or_file_size;
    uint8_t  data[UF2_PAYLOAD_MAX];
    uint32_t magic_end;
} uf2_block_t;

typedef enum {
    UF2_OK = 0,
    UF2_ERR_BAD_MAGIC,
    UF2_ERR_BAD_FLAGS,
    UF2_ERR_BAD_PAYLOAD,
    UF2_ERR_BAD_FAMILY,
    UF2_ERR_BAD_ADDRESS,
    UF2_ERR_OVERFLOW,
    UF2_ERR_BLOCK_INDEX,
    UF2_ERR_BLOCK_COUNT,
    UF2_ERR_MISSING_BLOCK,
    UF2_ERR_DUPLICATE_CONFLICT,
    UF2_ERR_TOO_MANY_BLOCKS,
    UF2_ERR_IMAGE_TOO_LARGE,
} uf2_result_t;

typedef struct {
    uintptr_t min_target;              /* inclusive */
    uintptr_t max_target_exclusive;    /* exclusive */
    size_t    max_image_size;
    bool      require_family_id;
    uint32_t  accepted_family;
} uf2_policy_t;

/* Compact accumulator — payloads live elsewhere (PSRAM / test buffer). */
typedef struct {
    uint32_t num_blocks;
    uint32_t family_id;
    uint32_t min_addr;
    uint32_t max_addr_exclusive;
    uint32_t total_payload;
    uint32_t blocks_accepted;
    uint8_t  seen[UF2_MAX_BLOCKS];
    uint32_t target_addr[UF2_MAX_BLOCKS];
    uint16_t payload_size[UF2_MAX_BLOCKS];
    uint32_t payload_crc[UF2_MAX_BLOCKS];
} uf2_accum_t;

void uf2_policy_default_sram(uf2_policy_t *policy);
uint32_t uf2_crc32(const uint8_t *data, uint32_t len);

uf2_result_t uf2_parse_block(const uint8_t raw[UF2_BLOCK_SIZE], uf2_block_t *out);

uf2_result_t uf2_accum_begin(uf2_accum_t *acc);
uf2_result_t uf2_accum_add(uf2_accum_t *acc, const uf2_block_t *blk,
                           const uf2_policy_t *policy);
uf2_result_t uf2_accum_finalize(const uf2_accum_t *acc);

const char *uf2_result_str(uf2_result_t r);

#endif
