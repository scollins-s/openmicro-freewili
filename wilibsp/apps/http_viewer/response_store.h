#ifndef RESPONSE_STORE_H
#define RESPONSE_STORE_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef struct response_store {
    uint8_t *body;
    uint32_t body_capacity;
    uint32_t body_length;

    uint32_t *line_offsets;
    uint16_t line_count;
    uint16_t line_capacity;

    uint16_t top_line;
    uint16_t cols; /* wrap width in characters */
    bool finalized;
    bool truncated;
    bool binary_rejected;
    char content_type[48];
    uint16_t http_status;
} response_store_t;

void response_store_init(response_store_t *s, uint8_t *body, uint32_t body_cap,
                         uint32_t *line_offsets, uint16_t line_cap, uint16_t cols);

void response_store_reset(response_store_t *s);

bool response_store_begin(response_store_t *s, uint16_t status, const char *ctype,
                          uint32_t declared_len);

bool response_store_append(response_store_t *s, const char *data, uint32_t len);

void response_store_finalize(response_store_t *s, bool truncated);

/* Rebuild line_offsets from body[0..body_length). */
void response_store_rewrap(response_store_t *s);

uint16_t response_store_page_lines(const response_store_t *s, uint16_t rows);

void response_store_page_prev(response_store_t *s, uint16_t rows);
void response_store_page_next(response_store_t *s, uint16_t rows);

/* Copy one display line (NUL-terminated, at most out_cap-1 chars). */
bool response_store_get_line(const response_store_t *s, uint16_t line_index,
                             char *out, size_t out_cap);

#endif
