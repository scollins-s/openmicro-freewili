#include "test_util.h"
#include "store_uart_proto.h"
#include <string.h>
#include <stdio.h>

static void test_crc_known(void) {
    /* CRC of empty = 0 */
    ASSERT_EQ(store_uart_crc32("", 0), 0u);
    const char *s = "123456789";
    ASSERT_EQ(store_uart_crc32(s, 9), 0xCBF43926u);
}

static void test_sha256_abc(void) {
    uint8_t out[32];
    store_sha256("abc", 3, out);
    static const uint8_t expect[32] = {
        0xba,0x78,0x16,0xbf,0x8f,0x01,0xcf,0xea,0x41,0x41,0x40,0xde,0x5d,0xae,0x22,0x23,
        0xb0,0x03,0x61,0xa3,0x96,0x17,0x7a,0x9c,0xb4,0x10,0xff,0x61,0xf2,0x00,0x15,0xad
    };
    ASSERT_TRUE(memcmp(out, expect, 32) == 0);
}

static void test_roundtrip_frame(void) {
    uint8_t enc[STORE_UART_MAX_FRAME];
    const char *payload = "hello-store";
    size_t n = store_uart_encode(STORE_UART_DATA, 0, 42, payload, (uint16_t)strlen(payload),
                                 enc, sizeof(enc));
    ASSERT_TRUE(n > 0);

    store_uart_parser_t p;
    store_uart_parser_init(&p);
    store_uart_frame_t fr;
    int got = 0;
    for (size_t i = 0; i < n; i++) {
        int r = store_uart_parser_feed(&p, enc[i], &fr);
        if (r == 1) {
            ASSERT_EQ(fr.type, STORE_UART_DATA);
            ASSERT_EQ(fr.seq, 42);
            ASSERT_EQ(fr.payload_len, (uint16_t)strlen(payload));
            ASSERT_TRUE(memcmp(fr.payload, payload, fr.payload_len) == 0);
            got = 1;
        }
    }
    ASSERT_TRUE(got);
}

static void test_resync_on_noise(void) {
    uint8_t enc[STORE_UART_MAX_FRAME];
    const char *payload = "x";
    size_t n = store_uart_encode(STORE_UART_ACK, 0, 1, payload, 1, enc, sizeof(enc));
    ASSERT_TRUE(n > 0);

    store_uart_parser_t p;
    store_uart_parser_init(&p);
    store_uart_frame_t fr;
    /* prefix noise */
    store_uart_parser_feed(&p, 0x00, &fr);
    store_uart_parser_feed(&p, 0xFF, &fr);
    int got = 0;
    for (size_t i = 0; i < n; i++) {
        if (store_uart_parser_feed(&p, enc[i], &fr) == 1) {
            ASSERT_EQ(fr.type, STORE_UART_ACK);
            got = 1;
        }
    }
    ASSERT_TRUE(got);
}

static void test_transfer_pipe_sha(void) {
    /* Simulate ESP hashing chunks the same way as END payload */
    const uint8_t chunk1[] = {1, 2, 3, 4};
    const uint8_t chunk2[] = {5, 6, 7, 8, 9};
    store_sha256_ctx ctx;
    store_sha256_init(&ctx);
    store_sha256_update(&ctx, chunk1, sizeof(chunk1));
    store_sha256_update(&ctx, chunk2, sizeof(chunk2));
    uint8_t stream[32];
    store_sha256_final(&ctx, stream);

    uint8_t oneshot[32];
    uint8_t all[9] = {1, 2, 3, 4, 5, 6, 7, 8, 9};
    store_sha256(all, sizeof(all), oneshot);
    ASSERT_TRUE(memcmp(stream, oneshot, 32) == 0);
}

int main(void) {
    test_crc_known();
    test_sha256_abc();
    test_roundtrip_frame();
    test_resync_on_noise();
    test_transfer_pipe_sha();
    printf("test_store_uart: OK\n");
    TEST_RETURN();
}
