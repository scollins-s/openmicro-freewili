# Display firmware (openmicro)

Snapshot of `device stuff/wilibsp/apps/openmicro` for convenient reading and
editing inside this kit.

## Build and flash

This app depends on the FreeWili display BSP. **Always build/flash through
wilibsp**, not as a standalone CMake project from this folder alone.

From the kit root:

```powershell
npm run flash:display
```

Sync this snapshot into wilibsp, then flash:

```powershell
powershell -File scripts/flash-display.ps1 -Sync
```

Or manually:

```powershell
cd "C:\repos\free wili 2\device stuff\wilibsp"
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

- **wilibsp app** is what actually gets compiled.
- **This folder** is the kit’s documented copy.
- After editing here, run `npm run sync:firmware` (or `-Sync` on flash) before
  flashing. After editing in wilibsp, copy back here if you want the kit
  snapshot updated.
