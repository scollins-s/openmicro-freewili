# Kit setup

Install once on a PC that will run OpenMicro and talk to FreeWili 2.

## Prerequisites

| Requirement | Notes |
|---|---|
| Node.js ≥ 22 | Host + launcher |
| Python 3 | `py -3` for wilibsp `fw.py` |
| Pico SDK / OpenOCD | Same install `wilibsp` already uses (`~/.pico-sdk` or PATH) |
| Claude and/or Codex CLI | On `PATH`; OpenMicro wraps one of them |
| FreeWili 2 + USB CMSIS-DAP | Display flash + RTT |
| This repo checkout | Kit expects sibling `device stuff/wilibsp` |

## Install the PC host

```powershell
cd "C:\repos\free wili 2\openmicro-freewili"
npm run setup
```

That runs `npm install` and `npm run build` inside `host/`.

## Flash the display app (once, or after UI changes)

Default flashes the app already in wilibsp (no sync):

```powershell
npm run flash:display
```

If you edited sources under `firmware/openmicro/` in this kit, sync then flash:

```powershell
npm run flash:display --  # or:
powershell -File scripts/flash-display.ps1 -Sync
```

Probe on the wrong CMSIS-DAP interface:

```powershell
powershell -File scripts/flash-display.ps1 -Iface 1
```

Build only (no flash):

```powershell
powershell -File scripts/flash-display.ps1 -BuildOnly
```

See [firmware/README.md](../firmware/README.md).

## Verify without hardware

```powershell
npm run freewili:sim
```

You should see the host start with FreeWili TCP and a simulated `prompt` action.

## Next

[RUN.md](RUN.md) — one-command stack, manual mode, CDC.
