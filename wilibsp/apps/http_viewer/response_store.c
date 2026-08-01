#include "response_store.h"

#include <stdio.h>
#include <string.h>

static int is_textish(const char *ctype) {
    if (!ctype || !ctype[0]) return 1; /* unknown → try text */
    if (strncmp(ctype, "text/", 5) == 0) return 1;
    if (strstr(ctype, "json") != NULL) return 1;
    if (strstr(ctype, "xml") != NULL) return 1;
    if (strstr(ctype, "javascript") != NULL) return 1;
    return 0;
}

void response_store_init(response_store_t *s, uint8_t *body, uint32_t body_cap,
                         uint32_t *line_offsets, uint16_t line_cap, uint16_t cols) {
    memset(s, 0, sizeof(*s));
    s->body = body;
    s->body_capacity = body_cap;
    s->line_offsets = line_offsets;
    s->line_capacity = line_cap;
    s->cols = cols ? cols : 76;
    if (s->body && s->body_capacity) s->body[0] = 0;
}

void response_store_reset(response_store_t *s) {
    s->body_length = 0;
    s->line_count = 0;
    s->top_line = 0;
    s->finalized = false;
    s->truncated = false;
    s->binary_rejected = false;
    s->http_status = 0;
    s->content_type[0] = 0;
    if (s->body && s->body_capacity) s->body[0] = 0;
}

bool response_store_begin(response_store_t *s, uint16_t status, const char *ctype,
                          uint32_t declared_len) {
    (void)declared_len;
    response_store_reset(s);
    s->http_status = status;
    snprintf(s->content_type, sizeof(s->content_type), "%s", ctype ? ctype : "");
    if (!is_textish(ctype)) {
        s->binary_rejected = true;
        char msg[128];
        snprintf(msg, sizeof(msg),
                 "Unsupported response type:\n%s\n", ctype ? ctype : "unknown");
        size_t n = strlen(msg);
        if (n > s->body_capacity) n = s->body_capacity;
        memcpy(s->body, msg, n);
        s->body_length = (uint32_t)n;
        response_store_rewrap(s);
        s->finalized = true;
        return false;
    }
    return true;
}

bool response_store_append(response_store_t *s, const char *data, uint32_t len) {
    if (!s || !s->body || s->binary_rejected || s->finalized) return false;
    if (!data || len == 0) return true;
    uint32_t room = s->body_capacity - s->body_length;
    if (len > room) {
        len = room;
        s->truncated = true;
    }
    if (len == 0) {
        s->truncated = true;
        return false;
    }
    /* Normalize CR/LF and drop NULs into the store. */
    for (uint32_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)data[i];
        if (c == '\r') continue;
        if (c == 0) c = '?';
        if (s->body_length >= s->body_capacity) {
            s->truncated = true;
            break;
        }
        s->body[s->body_length++] = c;
    }
    return !s->truncated;
}

void response_store_finalize(response_store_t *s, bool truncated) {
    if (!s) return;
    if (truncated) s->truncated = true;
    s->finalized = true;
    response_store_rewrap(s);
}

void response_store_rewrap(response_store_t *s) {
    if (!s || !s->line_offsets || s->line_capacity == 0) return;
    s->line_count = 0;
    if (s->body_length == 0) {
        s->line_offsets[0] = 0;
        s->line_count = 1;
        s->top_line = 0;
        return;
    }
    s->line_offsets[s->line_count++] = 0;
    uint16_t col = 0;
    for (uint32_t i = 0; i < s->body_length; i++) {
        unsigned char c = s->body[i];
        if (c == '\n') {
            col = 0;
            if (i + 1 < s->body_length && s->line_count < s->line_capacity)
                s->line_offsets[s->line_count++] = i + 1;
            continue;
        }
        col++;
        if (col >= s->cols) {
            col = 0;
            if (i + 1 < s->body_length && s->line_count < s->line_capacity)
                s->line_offsets[s->line_count++] = i + 1;
        }
    }
    if (s->top_line >= s->line_count)
        s->top_line = (uint16_t)(s->line_count - 1);
}

uint16_t response_store_page_lines(const response_store_t *s, uint16_t rows) {
    (void)s;
    return rows ? rows : 1;
}

void response_store_page_prev(response_store_t *s, uint16_t rows) {
    if (!s || rows == 0) return;
    if (s->top_line > rows) s->top_line = (uint16_t)(s->top_line - rows);
    else s->top_line = 0;
}

void response_store_page_next(response_store_t *s, uint16_t rows) {
    if (!s || rows == 0 || s->line_count == 0) return;
    uint16_t next = (uint16_t)(s->top_line + rows);
    if (next >= s->line_count) {
        if (s->line_count > rows) s->top_line = (uint16_t)(s->line_count - rows);
        else s->top_line = 0;
    } else {
        s->top_line = next;
    }
}

bool response_store_get_line(const response_store_t *s, uint16_t line_index,
                             char *out, size_t out_cap) {
    if (!s || !out || out_cap == 0 || line_index >= s->line_count) {
        if (out && out_cap) out[0] = 0;
        return false;
    }
    uint32_t start = s->line_offsets[line_index];
    uint32_t end = (line_index + 1 < s->line_count)
                       ? s->line_offsets[line_index + 1]
                       : s->body_length;
    if (end > start && s->body[end - 1] == '\n') end--;
    uint32_t n = end - start;
    if (n > s->cols) n = s->cols;
    if (n + 1 > out_cap) n = (uint32_t)(out_cap - 1);
    memcpy(out, s->body + start, n);
    out[n] = 0;
    return true;
}
