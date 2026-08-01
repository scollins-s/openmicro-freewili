#include "response_store.h"
#include <stdio.h>
#include <string.h>

static int fail(const char *m) {
    fprintf(stderr, "FAIL: %s\n", m);
    return 1;
}

int main(void) {
    uint8_t body[256];
    uint32_t lines[64];
    response_store_t s;
    response_store_init(&s, body, sizeof(body), lines, 64, 10);

    if (!response_store_begin(&s, 200, "text/plain", 0)) return fail("begin text");
    if (!response_store_append(&s, "hello\nworld-this-is-long\n", 24)) return fail("append");
    response_store_finalize(&s, false);
    if (s.line_count < 3) return fail("wrap lines");

    char line[32];
    if (!response_store_get_line(&s, 0, line, sizeof(line))) return fail("line0");
    if (strcmp(line, "hello") != 0) return fail("line0 content");

    response_store_reset(&s);
    if (!response_store_begin(&s, 200, "image/png", 100)) {
        /* binary rejected — expected false from begin after filling message */
    }
    if (!s.binary_rejected) return fail("binary reject");
    if (!s.finalized) return fail("binary finalized");

    /* Soft-wrap: 25 'a's with cols=10 → at least 3 lines */
    response_store_reset(&s);
    response_store_begin(&s, 200, "text/html", 0);
    char as[26];
    memset(as, 'a', 25);
    as[25] = 0;
    response_store_append(&s, as, 25);
    response_store_finalize(&s, false);
    if (s.line_count < 3) return fail("soft wrap");

    response_store_page_next(&s, 1);
    if (s.top_line != 1) return fail("page next");
    response_store_page_prev(&s, 1);
    if (s.top_line != 0) return fail("page prev");

    printf("response_store host test OK\n");
    return 0;
}
