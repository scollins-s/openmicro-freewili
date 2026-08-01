/* http_viewer — GET a compile-time URL via OneWili text cmds (a\h\*) and
 * show the response body. Power-aware: idle backlight-off, no LEDs, sleep
 * between polls. Requires main firmware with http-viewer-peer linked. */
#include "config.h"
#include "fw2.h"
#include "http_net.h"
#include "platform/diag.h"
#include "platform/psram.h"
#include "response_store.h"
#include "view.h"
#include "pico/stdlib.h"
#include <stdio.h>
#include <string.h>

#define LINE_CAP 1024u
#define COLS     76u

static http_net_t s_net;
static response_store_t s_store;
static view_state_t s_view;

/* Fallback if PSRAM init fails — small SRAM body for error messaging only. */
static uint8_t s_sram_body[2048];
static uint32_t s_sram_lines[128];

static bool on_begin(void *user, uint16_t status, const char *ctype, uint32_t clen) {
    (void)user;
    (void)clen;
    response_store_begin(&s_store, status, ctype, clen);
    s_view.dirty = true;
    return true;
}

static bool on_data(void *user, const char *chunk, uint32_t len) {
    (void)user;
    response_store_append(&s_store, chunk, len);
    response_store_rewrap(&s_store);
    s_view.dirty = true;
    return true;
}

static void on_end(void *user, uint32_t bytes, bool truncated) {
    (void)user;
    (void)bytes;
    response_store_finalize(&s_store, truncated);
    s_view.dirty = true;
}

static void wait_touch_release(void) {
    uint16_t x, y;
    absolute_time_t t0 = get_absolute_time();
    while (ft6336_poll(&x, &y)) {
        sleep_ms(20);
        if (absolute_time_diff_us(t0, get_absolute_time()) > 2000000) break;
    }
}

static void start_request(void) {
    response_store_reset(&s_store);
    http_net_request_get(&s_net, HTTP_VIEWER_URL);
    s_view.dirty = true;
}

int main(void) {
    board_init();
    st7796_init();
    board_backlight_set(1);
    ft6336_init();

    size_t ps = psram_init();
    uint8_t *body = s_sram_body;
    uint32_t body_cap = (uint32_t)sizeof(s_sram_body);
    uint32_t *lines = s_sram_lines;
    uint16_t line_cap = (uint16_t)(sizeof(s_sram_lines) / sizeof(s_sram_lines[0]));

    if (ps >= HTTP_VIEWER_PSRAM_OFFSET + HTTP_VIEWER_MAX_BODY_BYTES + LINE_CAP * 4u) {
        body = (uint8_t *)(PSRAM_BASE + HTTP_VIEWER_PSRAM_OFFSET);
        body_cap = HTTP_VIEWER_MAX_BODY_BYTES;
        lines = (uint32_t *)(PSRAM_BASE + HTTP_VIEWER_PSRAM_OFFSET + HTTP_VIEWER_MAX_BODY_BYTES);
        line_cap = (uint16_t)LINE_CAP;
        DIAG("http_viewer: PSRAM body %u @ +0x%x\n",
             (unsigned)body_cap, (unsigned)HTTP_VIEWER_PSRAM_OFFSET);
    } else {
        DIAG("http_viewer: PSRAM unavailable (%u) — SRAM fallback %u\n",
             (unsigned)ps, (unsigned)body_cap);
    }

    response_store_init(&s_store, body, body_cap, lines, line_cap, COLS);
    view_init(&s_view);

    s_net.store_user = &s_store;
    s_net.on_begin = on_begin;
    s_net.on_data = on_data;
    s_net.on_end = on_end;
    http_net_init(&s_net);

    DIAG("http_viewer: up url=%s idle_standby=%u ms\n",
         HTTP_VIEWER_URL, (unsigned)HTTP_VIEWER_IDLE_MS);

    view_draw(&s_view, &s_net, &s_store);

    bool auto_started = false;
    bool touching = false;
    absolute_time_t last_activity = get_absolute_time();
    absolute_time_t request_deadline = nil_time;
    absolute_time_t wifi_deadline = make_timeout_time_ms(HTTP_VIEWER_TIMEOUT_MS);

    for (;;) {
        http_net_poll(&s_net);

        /* No main peer / no httpWifi reply → surface error, then idle standby can dim. */
        if ((s_net.state == HTTP_NET_WIFI_CHECK || s_net.state == HTTP_NET_OPENING) &&
            !is_nil_time(wifi_deadline) &&
            absolute_time_diff_us(get_absolute_time(), wifi_deadline) <= 0) {
            DIAG("http_viewer: Wi-Fi/link timeout — is http-viewer-peer on main?\n");
            s_net.state = HTTP_NET_ERROR;
            snprintf(s_net.err, sizeof(s_net.err), "no main peer");
            snprintf(s_net.status, sizeof(s_net.status), "timeout — flash main peer?");
            wifi_deadline = nil_time;
            s_view.dirty = true;
        }

        /* One auto-GET after Wi-Fi OK — do not retry forever (power). */
        if (!auto_started && s_net.state == HTTP_NET_READY && s_net.wifi_ok) {
            auto_started = true;
            wifi_deadline = nil_time;
            start_request();
            request_deadline = make_timeout_time_ms(HTTP_VIEWER_TIMEOUT_MS);
        }

        if (s_net.state == HTTP_NET_REQUESTING || s_net.state == HTTP_NET_RECEIVING) {
            if (!is_nil_time(request_deadline) &&
                absolute_time_diff_us(get_absolute_time(), request_deadline) <= 0) {
                DIAG("http_viewer: request timeout\n");
                http_net_request_cancel(&s_net);
                s_net.state = HTTP_NET_ERROR;
                snprintf(s_net.err, sizeof(s_net.err), "timeout");
                snprintf(s_net.status, sizeof(s_net.status), "timeout — main peer?");
                request_deadline = nil_time;
                s_view.dirty = true;
            }
        } else {
            request_deadline = nil_time;
        }

        bool activity = false;
        uint16_t x, y;
        if (ft6336_poll(&x, &y)) {
            activity = true;
            if (s_view.asleep) {
                wait_touch_release();
                view_leave_standby(&s_view);
                touching = false;
            } else if (!touching) {
                touching = true;
                view_hit_t hit = view_hit(x, y);
                if (hit == VIEW_HIT_OFF) {
                    view_enter_standby(&s_view);
                } else if (hit == VIEW_HIT_PREV) {
                    response_store_page_prev(&s_store, VIEW_BODY_ROWS);
                    s_view.dirty = true;
                } else if (hit == VIEW_HIT_REFRESH) {
                    if (s_net.state == HTTP_NET_REQUESTING ||
                        s_net.state == HTTP_NET_RECEIVING) {
                        http_net_request_cancel(&s_net);
                    }
                    start_request();
                    request_deadline = make_timeout_time_ms(HTTP_VIEWER_TIMEOUT_MS);
                } else if (hit == VIEW_HIT_CANCEL) {
                    if (s_net.state == HTTP_NET_REQUESTING ||
                        s_net.state == HTTP_NET_RECEIVING) {
                        http_net_request_cancel(&s_net);
                        s_view.dirty = true;
                    } else {
                        response_store_page_next(&s_store, VIEW_BODY_ROWS);
                        s_view.dirty = true;
                    }
                }
            }
        } else {
            touching = false;
        }

        if (activity) last_activity = get_absolute_time();

        if (!s_view.asleep) {
            int64_t idle_us = absolute_time_diff_us(last_activity, get_absolute_time());
            if (idle_us >= (int64_t)HTTP_VIEWER_IDLE_MS * 1000) {
                view_enter_standby(&s_view);
            }
        }

        if (!s_view.asleep && s_view.dirty) {
            view_draw(&s_view, &s_net, &s_store);
        } else if (s_view.asleep) {
            s_view.dirty = false;
        }

        uint32_t poll_ms = HTTP_VIEWER_POLL_ACTIVE_MS;
        if (s_view.asleep) {
            poll_ms = HTTP_VIEWER_POLL_SLEEP_MS;
        } else {
            int64_t quiet_us = absolute_time_diff_us(last_activity, get_absolute_time());
            if (quiet_us >= (int64_t)HTTP_VIEWER_QUIET_MS * 1000)
                poll_ms = HTTP_VIEWER_POLL_QUIET_MS;
        }
        sleep_ms(poll_ms);
    }
}
