#include "http_net.h"

#include "onewili.h"
#include "onewili_fwgui.h"
#include "platform/diag.h"

#include <stdio.h>
#include <string.h>

/*
 * Display --FwGUI/OneWili--> Main --ESP HTTPS--> web
 * Wire paths: docs/http-viewer/ONEWILI-HTTP-VIEWER-COMMANDS.md
 */

static ow_device s_dev;
static bool s_opened;

static void set_status(http_net_t *n, const char *s) {
    snprintf(n->status, sizeof(n->status), "%s", s ? s : "");
}

static void set_err(http_net_t *n, const char *s) {
    snprintf(n->err, sizeof(n->err), "%s", s ? s : "");
    n->state = HTTP_NET_ERROR;
    set_status(n, n->err);
    DIAG("http_net: %s\n", n->err);
}

static bool send_wire(http_net_t *n, const char *wire) {
    if (!s_opened) {
        set_err(n, "OneWili not open");
        return false;
    }
    const ow_transport *t = &s_dev.t;
    if (!t->write) {
        set_err(n, "No OneWili transport");
        return false;
    }
    size_t len = strlen(wire);
    if (len + 2 > 240) {
        set_err(n, "Wire cmd too long");
        return false;
    }
    uint8_t out[256];
    out[0] = 0x02;
    memcpy(out + 1, wire, len);
    out[1 + len] = '\n';
    s_dev.line_len = 0;
    if (t->write(t->ctx, out, len + 2) < 0) {
        set_err(n, "OneWili write failed");
        return false;
    }
    DIAG("http_net: sent [%s]\n", wire);
    return true;
}

void http_net_init(http_net_t *n) {
    memset(n, 0, sizeof(*n));
    n->state = HTTP_NET_OPENING;
    set_status(n, "Opening OneWili...");
    s_opened = false;
    memset(&s_dev, 0, sizeof(s_dev));
    if (ow_open_fwgui(&s_dev) != OW_OK) {
        set_err(n, "ow_open_fwgui failed");
        return;
    }
    s_opened = true;
    n->state = HTTP_NET_WIFI_CHECK;
    set_status(n, "Checking Wi-Fi...");
    DIAG("http_net: OneWili open OK\n");
    http_net_request_wifi(n);
}

static void on_event(http_net_t *n, const char *id, const char *args) {
    if (!id) return;
    if (strcmp(id, "httpWifi") == 0) {
        n->wifi_known = true;
        n->wifi_ok = args && args[0] == '1';
        DIAG("http_net: wifi %s\n", n->wifi_ok ? "ok" : "down");
        if (n->state == HTTP_NET_WIFI_CHECK || n->state == HTTP_NET_OPENING) {
            if (n->wifi_ok) {
                n->state = HTTP_NET_READY;
                set_status(n, "Wi-Fi OK — ready");
            } else {
                set_err(n, "Wi-Fi not connected");
            }
        }
    } else if (strcmp(id, "httpBegin") == 0) {
        unsigned st = 0, clen = 0;
        char ctype[48];
        ctype[0] = 0;
        if (args) sscanf(args, "%u %47s %u", &st, ctype, &clen);
        n->http_status = (uint16_t)st;
        n->content_length = clen;
        n->bytes_received = 0;
        n->truncated = false;
        snprintf(n->content_type, sizeof(n->content_type), "%s", ctype);
        n->state = HTTP_NET_RECEIVING;
        snprintf(n->status, sizeof(n->status), "HTTP %u receiving...", st);
        if (n->on_begin) n->on_begin(n->store_user, n->http_status, n->content_type, clen);
    } else if (strcmp(id, "httpData") == 0) {
        const char *chunk = args ? args : "";
        uint32_t len = (uint32_t)strlen(chunk);
        n->bytes_received += len;
        n->state = HTTP_NET_RECEIVING;
        if (n->content_length > 0) {
            snprintf(n->status, sizeof(n->status), "Receiving %u / %u",
                     (unsigned)n->bytes_received, (unsigned)n->content_length);
        } else {
            snprintf(n->status, sizeof(n->status), "Receiving %u bytes",
                     (unsigned)n->bytes_received);
        }
        if (n->on_data) n->on_data(n->store_user, chunk, len);
    } else if (strcmp(id, "httpEnd") == 0) {
        unsigned bytes = 0, trunc = 0;
        if (args) sscanf(args, "%u %u", &bytes, &trunc);
        n->bytes_received = bytes ? bytes : n->bytes_received;
        n->truncated = trunc != 0;
        n->state = HTTP_NET_COMPLETE;
        if (n->truncated) {
            snprintf(n->status, sizeof(n->status), "Truncated at %u bytes",
                     (unsigned)n->bytes_received);
        } else {
            snprintf(n->status, sizeof(n->status), "Done %u bytes HTTP %u",
                     (unsigned)n->bytes_received, (unsigned)n->http_status);
        }
        if (n->on_end) n->on_end(n->store_user, n->bytes_received, n->truncated);
    } else if (strcmp(id, "httpStat") == 0) {
        snprintf(n->status, sizeof(n->status), "Main: %s", args ? args : "");
    } else if (strcmp(id, "httpErr") == 0) {
        set_err(n, args && args[0] ? args : "httpErr");
    }
}

void http_net_poll(http_net_t *n) {
    if (!s_opened) return;
    char id[64];
    char args[512];
    for (;;) {
        int r = ow_poll_text_line(&s_dev, id, sizeof(id), args, sizeof(args));
        if (r != 1) break;
        on_event(n, id, args);
    }
}

bool http_net_request_wifi(http_net_t *n) {
    set_status(n, "Checking Wi-Fi...");
    return send_wire(n, "a\\h\\w");
}

bool http_net_request_get(http_net_t *n, const char *url) {
    if (!url || !url[0]) return false;
    char cmd[240];
    snprintf(cmd, sizeof(cmd), "a\\h\\g %s", url);
    n->state = HTTP_NET_REQUESTING;
    n->bytes_received = 0;
    n->truncated = false;
    n->http_status = 0;
    set_status(n, "Requesting...");
    return send_wire(n, cmd);
}

bool http_net_request_cancel(http_net_t *n) {
    set_status(n, "Cancelling...");
    return send_wire(n, "a\\h\\c");
}

bool http_net_request_status(http_net_t *n) {
    return send_wire(n, "a\\h\\s");
}

const char *http_net_status(const http_net_t *n) {
    return n->status;
}
