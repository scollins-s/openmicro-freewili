# Display firmware (openmicro)

Editable source for the OpenMicro display application. The matching FreeWili 2
BSP and OneWili library are vendored in this repository under `wilibsp/` and
`onewili/`.

## Build and flash

This app depends on the FreeWili display BSP. Build and flash through the
vendored `wilibsp`; the wrapper below synchronizes this directory first.

From the kit root:

```powershell
npm run flash:display
```

Synchronize without building or flashing:

```powershell
powershell -File scripts/flash-display.ps1 -SyncOnly
```

Or manually:

```powershell
cd wilibsp
py -3 tools\fw.py flash openmicro
```

## Behaviour

| Mode | When | Behaviour |
|---|---|---|
| Offline demo | `ow_open_fwgui` fails | Local UI state cycling |
| Linked | FwGUI up | Taps send `a\om\*` + log `OMTX …` on RTT |
| Standby | 30 s idle | Backlight off; tap wakes |

Board → Claude today: keep OpenMicro `--freewili` running, then OpenOCD RTT +
RTT bridge (or `npm start` from the kit root).

## Keep copies in sync

- **This folder is the source of truth.**
- `wilibsp/apps/openmicro` is the synchronized build copy.
- `npm run flash:display` and `-BuildOnly` synchronize automatically.
- Avoid editing the synchronized copy because the next build overwrites it.
