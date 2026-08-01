#include "uf2_reader.h"
#include <stdio.h>
#include <string.h>

static int fails;

static void expect(int cond, const char *msg) {
    if (!cond) {
        printf("FAIL: %s\n", msg);
        fails++;
    }
}

static void fill_block(uf2_block_t *b, uint32_t addr, uint32_t size,
                       uint32_t no, uint32_t nblocks, uint32_t family) {
    memset(b, 0, sizeof(*b));
    b->magic_start0 = UF2_MAGIC_START0;
    b->magic_start1 = UF2_MAGIC_START1;
    b->flags = UF2_FLAG_FAMILY;
    b->target_addr = addr;
    b->payload_size = size;
    b->block_no = no;
    b->num_blocks = nblocks;
    b->family_or_file_size = family;
    for (uint32_t i = 0; i < size; i++) b->data[i] = (uint8_t)(i ^ no);
    b->magic_end = UF2_MAGIC_END;
}

static void test_happy_path(void) {
    uf2_policy_t pol;
    uf2_policy_default_sram(&pol);
    uf2_accum_t acc;
    uf2_accum_begin(&acc);

    uf2_block_t b0, b1;
    fill_block(&b0, 0x20000000u, 256, 0, 2, UF2_FAMILY_RP2350_ARM_S);
    fill_block(&b1, 0x20000100u, 256, 1, 2, UF2_FAMILY_RP2350_ARM_S);
    expect(uf2_accum_add(&acc, &b0, &pol) == UF2_OK, "add b0");
    expect(uf2_accum_add(&acc, &b1, &pol) == UF2_OK, "add b1");
    expect(uf2_accum_finalize(&acc) == UF2_OK, "finalize");
    expect(acc.total_payload == 512, "payload sum");
}

static void test_flash_addr_rejected(void) {
    uf2_policy_t pol;
    uf2_policy_default_sram(&pol);
    uf2_accum_t acc;
    uf2_accum_begin(&acc);
    uf2_block_t b;
    fill_block(&b, 0x10000000u, 256, 0, 1, UF2_FAMILY_RP2350_ARM_S);
    expect(uf2_accum_add(&acc, &b, &pol) == UF2_ERR_BAD_ADDRESS, "flash addr");
}

static void test_wrong_family(void) {
    uf2_policy_t pol;
    uf2_policy_default_sram(&pol);
    uf2_accum_t acc;
    uf2_accum_begin(&acc);
    uf2_block_t b;
    fill_block(&b, 0x20000000u, 256, 0, 1, 0x12345678u);
    expect(uf2_accum_add(&acc, &b, &pol) == UF2_ERR_BAD_FAMILY, "family");
}

static void test_missing_block(void) {
    uf2_policy_t pol;
    uf2_policy_default_sram(&pol);
    uf2_accum_t acc;
    uf2_accum_begin(&acc);
    uf2_block_t b;
    fill_block(&b, 0x20000000u, 256, 0, 2, UF2_FAMILY_RP2350_ARM_S);
    expect(uf2_accum_add(&acc, &b, &pol) == UF2_OK, "add only 0");
    expect(uf2_accum_finalize(&acc) == UF2_ERR_MISSING_BLOCK, "missing 1");
}

static void test_duplicate_conflict(void) {
    uf2_policy_t pol;
    uf2_policy_default_sram(&pol);
    uf2_accum_t acc;
    uf2_accum_begin(&acc);
    uf2_block_t b0, b1;
    fill_block(&b0, 0x20000000u, 256, 0, 1, UF2_FAMILY_RP2350_ARM_S);
    fill_block(&b1, 0x20000100u, 256, 0, 1, UF2_FAMILY_RP2350_ARM_S);
    expect(uf2_accum_add(&acc, &b0, &pol) == UF2_OK, "first");
    expect(uf2_accum_add(&acc, &b1, &pol) == UF2_ERR_DUPLICATE_CONFLICT, "conflict");
}

static void test_parse_roundtrip(void) {
    uf2_block_t src, out;
    fill_block(&src, 0x20001000u, 100, 3, 4, UF2_FAMILY_RP2350_ARM_S);
    uint8_t raw[UF2_BLOCK_SIZE];
    memcpy(raw, &src, sizeof(src));
    expect(uf2_parse_block(raw, &out) == UF2_OK, "parse");
    expect(out.target_addr == 0x20001000u, "addr");
    expect(out.block_no == 3, "no");
}

int main(void) {
    test_happy_path();
    test_flash_addr_rejected();
    test_wrong_family();
    test_missing_block();
    test_duplicate_conflict();
    test_parse_roundtrip();
    if (fails) {
        printf("%d failures\n", fails);
        return 1;
    }
    printf("uf2_reader: ok\n");
    return 0;
}
