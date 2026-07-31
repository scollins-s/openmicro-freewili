# openmicro (display firmware)

480×320 touch remote for OpenMicro on FreeWili 2. Custom UI (no LVGL): status
strip, Accept/Reject/Voice/New, workflow row, session chips, thinking ±, layer,
small d-pad.

**This is a kit snapshot.** Build/flash from the kit root
([`../README.md`](../README.md), `npm run flash:display`) or the live BSP app at
`device stuff/wilibsp/apps/openmicro/`. See also [`../README.md`](../README.md)
in `firmware/`.

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
| Standby | 30 s no input | Backlight off, LEDs blank, ~80 ms poll; tap/button wakes |

**Board → Claude:** keep OpenMicro `--freewili` running, then `fw rtt` +
`npm run bridge:rtt` in `demo_inspo/OpenMicro` (see FREEWILI.md).

Proposed OneWili paths:
[`docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md`](../../../../docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md).

PC host: `openmicro --freewili --no-hid` (see `demo_inspo/OpenMicro/FREEWILI.md`).

## Power / performance

- Backlight off after 30 s idle (largest savings on this board).
- WS2812 only updated when UI state is dirty (not on a timer); brightness 24/255.
- Poll ~16 ms when active, ~40 ms when quiet, ~80 ms in standby.
- Haptic only on primary/workflow taps (not d-pad / thinking / session chips).

## LEDs

WS2812 ×16: pixel 0 = status/lightbar color; pixels 1–5 = session slots
(focus highlighted); 6–15 stay off.
