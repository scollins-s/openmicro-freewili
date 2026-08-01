#include "uf2_reader.h"

#include <string.h>

/* RP2350 SRAM window used by Pico SDK no_flash / copy_to_ram images. */
#define DEFAULT_SRAM_MIN  0x20000000u
#define DEFAULT_SRAM_MAX  0x20082000u  /* exclusive; matches fw.py / RP2350B map */

void uf2_policy_default_sram(uf2_policy_t *policy) {
    if (!policy) return;
    memset(policy, 0, sizeof(*policy));
    policy->min_target = DEFAULT_SRAM_MIN;
    policy->max_target_exclusive = DEFAULT_SRAM_MAX;
    policy->max_image_size = DEFAULT_SRAM_MAX - DEFAULT_SRAM_MIN;
    policy->require_family_id = true;
    policy->accepted_family = UF2_FAMILY_RP2350_ARM_S;
}

uint32_t uf2_crc32(const uint8_t *data, uint32_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            uint32_t mask = -(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static bool family_ok(const uf2_policy_t *policy, uint32_t family) {
    if (!policy->require_family_id) return true;
    if (family == policy->accepted_family) return true;
    if (family == UF2_FAMILY_RP2350_ABSOLUTE) return true;
    if (family == UF2_FAMILY_RP2350_ARM_S) return true;
    return false;
}

static bool addr_in_range(const uf2_policy_t *policy, uint32_t addr, uint32_t size) {
    if (size == 0 || size > UF2_PAYLOAD_MAX) return false;
    uint64_t end = (uint64_t)addr + (uint64_t)size;
    if (end < addr) return false;
    if (addr < (uint32_t)policy->min_target) return false;
    if (end > (uint64_t)policy->max_target_exclusive) return false;
    return true;
}

uf2_result_t uf2_parse_block(const uint8_t raw[UF2_BLOCK_SIZE], uf2_block_t *out) {
    if (!raw || !out) return UF2_ERR_BAD_MAGIC;
    memcpy(out, raw, sizeof(*out));
    if (out->magic_start0 != UF2_MAGIC_START0) return UF2_ERR_BAD_MAGIC;
    if (out->magic_start1 != UF2_MAGIC_START1) return UF2_ERR_BAD_MAGIC;
    if (out->magic_end != UF2_MAGIC_END) return UF2_ERR_BAD_MAGIC;
    if (out->payload_size == 0 || out->payload_size > UF2_PAYLOAD_MAX) {
        return UF2_ERR_BAD_PAYLOAD;
    }
    return UF2_OK;
}

uf2_result_t uf2_accum_begin(uf2_accum_t *acc) {
    if (!acc) return UF2_ERR_BAD_MAGIC;
    memset(acc, 0, sizeof(*acc));
    acc->min_addr = 0xFFFFFFFFu;
    return UF2_OK;
}

uf2_result_t uf2_accum_add(uf2_accum_t *acc, const uf2_block_t *blk,
                           const uf2_policy_t *policy) {
    if (!acc || !blk || !policy) return UF2_ERR_BAD_MAGIC;

    if ((blk->flags & ~UF2_FLAG_FAMILY) != 0) return UF2_ERR_BAD_FLAGS;

    if (policy->require_family_id) {
        if ((blk->flags & UF2_FLAG_FAMILY) == 0) return UF2_ERR_BAD_FAMILY;
        if (!family_ok(policy, blk->family_or_file_size)) return UF2_ERR_BAD_FAMILY;
    }
    if (!addr_in_range(policy, blk->target_addr, blk->payload_size)) {
        return UF2_ERR_BAD_ADDRESS;
    }
    if (blk->num_blocks == 0 || blk->num_blocks > UF2_MAX_BLOCKS) {
        return UF2_ERR_TOO_MANY_BLOCKS;
    }
    if (blk->block_no >= blk->num_blocks) return UF2_ERR_BLOCK_INDEX;

    if (acc->num_blocks == 0) {
        acc->num_blocks = blk->num_blocks;
        acc->family_id = blk->family_or_file_size;
    } else {
        if (blk->num_blocks != acc->num_blocks) return UF2_ERR_BLOCK_COUNT;
        if ((blk->flags & UF2_FLAG_FAMILY) &&
            blk->family_or_file_size != acc->family_id) {
            return UF2_ERR_BAD_FAMILY;
        }
    }

    uint32_t crc = uf2_crc32(blk->data, blk->payload_size);
    if (acc->seen[blk->block_no]) {
        if (acc->target_addr[blk->block_no] != blk->target_addr ||
            acc->payload_size[blk->block_no] != blk->payload_size ||
            acc->payload_crc[blk->block_no] != crc) {
            return UF2_ERR_DUPLICATE_CONFLICT;
        }
        return UF2_OK;
    }

    uint64_t new_total = (uint64_t)acc->total_payload + blk->payload_size;
    if (new_total > policy->max_image_size) return UF2_ERR_IMAGE_TOO_LARGE;

    acc->seen[blk->block_no] = 1;
    acc->target_addr[blk->block_no] = blk->target_addr;
    acc->payload_size[blk->block_no] = (uint16_t)blk->payload_size;
    acc->payload_crc[blk->block_no] = crc;
    acc->total_payload = (uint32_t)new_total;
    acc->blocks_accepted++;

    if (blk->target_addr < acc->min_addr) acc->min_addr = blk->target_addr;
    uint32_t end = blk->target_addr + blk->payload_size;
    if (end > acc->max_addr_exclusive) acc->max_addr_exclusive = end;
    return UF2_OK;
}

uf2_result_t uf2_accum_finalize(const uf2_accum_t *acc) {
    if (!acc) return UF2_ERR_BAD_MAGIC;
    if (acc->num_blocks == 0) return UF2_ERR_MISSING_BLOCK;
    for (uint32_t i = 0; i < acc->num_blocks; i++) {
        if (!acc->seen[i]) return UF2_ERR_MISSING_BLOCK;
    }
    if (acc->min_addr >= acc->max_addr_exclusive) return UF2_ERR_BAD_ADDRESS;
    return UF2_OK;
}

const char *uf2_result_str(uf2_result_t r) {
    switch (r) {
    case UF2_OK: return "ok";
    case UF2_ERR_BAD_MAGIC: return "bad UF2 magic";
    case UF2_ERR_BAD_FLAGS: return "unsupported UF2 flags";
    case UF2_ERR_BAD_PAYLOAD: return "bad payload size";
    case UF2_ERR_BAD_FAMILY: return "wrong / missing family ID";
    case UF2_ERR_BAD_ADDRESS: return "target outside allowed SRAM";
    case UF2_ERR_OVERFLOW: return "address overflow";
    case UF2_ERR_BLOCK_INDEX: return "block index out of range";
    case UF2_ERR_BLOCK_COUNT: return "inconsistent block count";
    case UF2_ERR_MISSING_BLOCK: return "missing UF2 block";
    case UF2_ERR_DUPLICATE_CONFLICT: return "conflicting duplicate block";
    case UF2_ERR_TOO_MANY_BLOCKS: return "too many blocks";
    case UF2_ERR_IMAGE_TOO_LARGE: return "image too large";
    default: return "unknown UF2 error";
    }
}
