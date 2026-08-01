# http_viewer

Display-side HTTP text viewer for FreeWili 2. Performs a GET against a
compile-time URL over interim OneWili text commands (`a\h\*`), using the
device’s **saved Wi‑Fi** (no credentials in this app).

Requires main firmware with [`http-viewer-peer`](../../../http-viewer-peer/)
linked and ESP HTTPS (`http_fetch` / store-agent) available.

## URL

```c
https://webhook.site/6b9506f1-12cc-4c14-a0d0-5fa2b419c52c
```

Override at build time with `-DHTTP_VIEWER_URL=\"https://...\"` if needed.

## Power policy (do not flash until reviewed)

| Measure | Behavior |
|---|---|
| Backlight | Off at `board_init`; on only while UI awake |
| Idle standby | Backlight off + black screen after **30 s** inactivity |
| Wake | Tap screen (wake tap does not fire a button) |
| OFF button | Immediate standby |
| Poll sleep | 16 ms active / 40 ms quiet / 80 ms standby |
| LEDs | Not used |
| USB / radio / mics / IR | Not powered by this app |
| Network | One auto-GET after Wi‑Fi OK; Refresh only on demand; 15 s timeout |
| Body cap | 16 KiB in PSRAM (2 KiB SRAM fallback) |

This matches the `openmicro` idle pattern and avoids busy-spin or always-on
backlight.

## Build

From `wilibsp`:

```bash
fw build http_viewer
```

**Do not flash** until the power policy above is accepted.

## Protocol

See [ONEWILI-HTTP-VIEWER-COMMANDS.md](../../../../docs/http-viewer/ONEWILI-HTTP-VIEWER-COMMANDS.md).
