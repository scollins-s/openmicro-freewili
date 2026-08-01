#ifndef HTTP_VIEWER_CONFIG_H
#define HTTP_VIEWER_CONFIG_H

/* Compile-time GET target (owned by display firmware). */
#ifndef HTTP_VIEWER_URL
#define HTTP_VIEWER_URL \
    "https://webhook.site/6b9506f1-12cc-4c14-a0d0-5fa2b419c52c"
#endif

#ifndef HTTP_VIEWER_TIMEOUT_MS
#define HTTP_VIEWER_TIMEOUT_MS 15000u
#endif

#ifndef HTTP_VIEWER_MAX_BODY_BYTES
#define HTTP_VIEWER_MAX_BODY_BYTES (16u * 1024u)
#endif

#ifndef HTTP_VIEWER_USER_AGENT
#define HTTP_VIEWER_USER_AGENT "FreeWili2-HTTP-Viewer/0.1"
#endif

/* PSRAM region for body + line index (avoid capture @ 1 MB). */
#ifndef HTTP_VIEWER_PSRAM_OFFSET
#define HTTP_VIEWER_PSRAM_OFFSET 0x200000u
#endif

/* Power: backlight off after idle; sleep between polls. */
#ifndef HTTP_VIEWER_IDLE_MS
#define HTTP_VIEWER_IDLE_MS 30000u
#endif
#ifndef HTTP_VIEWER_POLL_ACTIVE_MS
#define HTTP_VIEWER_POLL_ACTIVE_MS 16u
#endif
#ifndef HTTP_VIEWER_POLL_QUIET_MS
#define HTTP_VIEWER_POLL_QUIET_MS 40u
#endif
#ifndef HTTP_VIEWER_POLL_SLEEP_MS
#define HTTP_VIEWER_POLL_SLEEP_MS 80u
#endif
#ifndef HTTP_VIEWER_QUIET_MS
#define HTTP_VIEWER_QUIET_MS 2000u
#endif

#endif
