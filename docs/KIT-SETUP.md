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
| This repo checkout | Includes the matching `wilibsp` and OneWili sources |

## Install the PC host

```powershell
cd "C:\repos\free wili 2\openmicro-freewili"
npm run setup
```

That runs `npm install` and `npm run build` inside `host/`.

## Flash the display app (once, or after UI changes)

The wrapper synchronizes `firmware/openmicro` into the vendored BSP and flashes
it:

```powershell
npm run flash:display
```

To synchronize without building or flashing:

```powershell
npm run sync:firmware
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
