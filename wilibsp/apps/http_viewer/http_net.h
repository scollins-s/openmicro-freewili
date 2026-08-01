#ifndef HTTP_NET_H
#define HTTP_NET_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    HTTP_NET_OPENING = 0,
    HTTP_NET_WIFI_CHECK,
    HTTP_NET_READY,
    HTTP_NET_REQUESTING,
    HTTP_NET_RECEIVING,
    HTTP_NET_COMPLETE,
    HTTP_NET_ERROR,
} http_net_state_t;

typedef struct http_net {
    http_net_state_t state;
    bool wifi_ok;
    bool wifi_known;
    uint16_t http_status;
    uint32_t content_length; /* 0 = unknown */
    uint32_t bytes_received;
    bool truncated;
    char content_type[48];
    char status[80];
    char err[96];
    /* Filled by httpData into caller-owned store via callbacks below. */
    void *store_user;
    bool (*on_begin)(void *user, uint16_t status, const char *ctype, uint32_t clen);
    bool (*on_data)(void *user, const char *chunk, uint32_t len);
    void (*on_end)(void *user, uint32_t bytes, bool truncated);
} http_net_t;

void http_net_init(http_net_t *n);
void http_net_poll(http_net_t *n);

bool http_net_request_wifi(http_net_t *n);
bool http_net_request_get(http_net_t *n, const char *url);
bool http_net_request_cancel(http_net_t *n);
bool http_net_request_status(http_net_t *n);

const char *http_net_status(const http_net_t *n);

#endif
