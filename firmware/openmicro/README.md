# openmicro (display firmware)

480×320 touch remote for OpenMicro on FreeWili 2. Custom UI (no LVGL): status
strip, Accept/Reject/Voice/New, workflow row, session chips, thinking ±, layer,
small d-pad.

The matching BSP is vendored with this repository. Build/flash from the kit
root ([`../README.md`](../README.md), `npm run flash:display`). The wrapper
synchronizes this source into `wilibsp/apps/openmicro/` before compiling.

## Build

From `wilibsp` (Pico SDK / VS Code Pico extension):

```text
apps/openmicro → target `openmicro`
```

Flash the `.uf2` to the **display** CPU. Keep stock firmware on **main** if
you want FwGUI/OneWili.

## Behaviour

| Mode | When | Behaviour |
|---|---|---|
| Offline demo | `ow_open_fwgui` fails | Cycles idle→exec→wait→done→err; taps update local UI |
| Linked | FwGUI up | Taps send `a\om\*` wires + log `OMTX …` on RTT |
| Quiet idle | 2 s no input | Screen remains on; input polls at a lower rate |

**Board → Claude:** from the repository root, run `npm start`. It launches the
included OpenMicro host, OpenOCD RTT server, and RTT-to-TCP bridge.

PC host details: [`../../host/FREEWILI.md`](../../host/FREEWILI.md).

## Power / performance

- Automatic backlight-off standby is disabled because some boards fail to wake
  from it without a power cycle. Custom builds can opt back in with
  `-DOM_IDLE_MS=<milliseconds>`.
- WS2812 only updated when UI state is dirty (not on a timer); brightness 24/255.
- Poll ~16 ms when active and ~40 ms when quiet.
- Haptic only on primary/workflow taps (not d-pad / thinking / session chips).

## LEDs

WS2812 ×16: pixel 0 = status/lightbar color; pixels 1–5 = session slots
(focus highlighted); 6–15 stay off.
