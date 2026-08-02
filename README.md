# OpenMicro FreeWili kit

Portable, one-folder setup for driving **OpenMicro** (Claude/Codex on the PC)
from a **FreeWili 2** touchscreen. The display BSP, button/haptic drivers,
OneWili transport, host, and launcher are included in this checkout.

The original multi-folder layout is unchanged — see
[`docs/openmicro-freewili/README.md`](../docs/openmicro-freewili/README.md).
This kit is a copy plus a **one-command launcher** so you do not need three
separate terminals for the PC side.

## What it can do

| Today (RTT MVP) | Future (CDC) |
|---|---|
| Touch Accept / Reject / Voice / workflows on the display | Same UI |
| Taps reach the PC over the debug probe (OpenOCD RTT) | Taps over USB CDC (no probe) |
| OpenMicro wraps Claude or Codex on the PC | Same |
| Feedback JSON over TCP to a device peer | Feedback over CDC via main peer |

Details: [docs/CAPABILITIES.md](docs/CAPABILITIES.md).

## Layout

```text
openmicro-freewili/
  host/                 OpenMicro PC host
  firmware/openmicro/   Canonical display app sources
  wilibsp/              Vendored FreeWili 2 display BSP and build tools
  onewili/              Vendored OneWili FwGUI transport library
  peer/                 Main-CPU CDC bridge kit (optional / future)
  scripts/start.mjs     One-command: OpenOCD + host + RTT bridge
  scripts/flash-display.ps1
  docs/                 Setup and run guides
```

## Quick start

**Prereqs:** Node.js ≥ 22, Python 3, CMake, Ninja, Pico SDK 2.x with an ARM GCC
toolchain, Claude or Codex CLI on `PATH`, and FreeWili 2 + CMSIS-DAP. The Pico
VS Code extension installation under `~/.pico-sdk` is detected automatically.

```powershell
cd <path-to-your-clone>
npm run setup
npm run flash:display          # once — probe connected, display CPU
npm start                      # ONE terminal: OpenOCD + host + bridge
```

Tap **Accept** on the device to send Enter to the visible agent. Normal startup
keeps OpenOCD/RTT diagnostics out of the agent UI; add `--show-rtt` when you
want to inspect the bridge traffic.

No hardware:

```powershell
npm run freewili:sim
```

More: [docs/KIT-SETUP.md](docs/KIT-SETUP.md) · [docs/RUN.md](docs/RUN.md).

## Demo portability

The demo does not require sibling repositories or absolute source paths. Clone
or copy the entire repository to any location, then run `npm run setup` and
`npm run flash:display`. Generated firmware builds stay under
`wilibsp/build/` and are not committed.

The following remain machine prerequisites rather than vendored files:

- Node.js 22 or newer and Python 3.
- Pico SDK 2.x, ARM GCC, CMake, Ninja, and OpenOCD. A Raspberry Pi Pico VS Code
  extension install under `~/.pico-sdk` is discovered automatically.
- A Claude or Codex CLI on `PATH` for the interactive agent.
- A connected FreeWili 2 CMSIS-DAP probe for flashing and the RTT demo.

The one-command flash wrapper is tested on Windows PowerShell. On macOS/Linux,
use `python3 wilibsp/tools/fw.py build openmicro` and `flash openmicro`, or run
the PowerShell wrapper with PowerShell 7 (`pwsh`).

The vendored BSP contains other experimental applications. App-store targets
and tests are automatically skipped unless their optional `store_uart` and
`app-store-online` sibling sources are present; OpenMicro does not require
either dependency.

## Commands

| Script | Purpose |
|---|---|
| `npm run setup` | `npm install` + build in `host/` |
| `npm start` / `npm run freewili` | One-command FreeWili stack |
| `npm run freewili:sim` | Host + fake device (no OpenOCD) |
| `npm run flash:display` | Flash display via wilibsp |
| `npm run sync:firmware` | Copy kit `firmware/openmicro` → wilibsp app |
| `npm run stop` | Kill leftovers on ports 48763 / 9090 |

Launcher flags (after `--`):

```powershell
npm start -- --agent codex
npm start -- --cwd C:\work\another-project
npm start -- --show-rtt       # show OpenOCD + RTT bridge diagnostics
npm start -- --iface 1
npm start -- --no-rtt          # host only
npm start -- --sim
```

`--cwd` (also `--project`) selects the coding agent's working directory; it
does not need to be inside this checkout. Set `OPENMICRO_CWD` to make that
directory the default for `npm start`.

### Display session and command buttons

- **S1–S5** select one of the five currently tracked OpenMicro sessions. The
  selected session receives Accept, Reject, prompts, arrows, and slash-command
  actions. A dim slot has no tracked session, and tapping it has no host target.
- **NEW** clears the current conversation (`/clear` in Claude, `/new` in Codex).
- **MODEL** clears the composer and opens `/model`.
- **RESUME** clears the composer and opens `/resume`, where the agent can select
  a previous session.

There is no S6 button in the current firmware/protocol UI; the six numbered
items are the configurable layers (`L1`–`L6`), while session slots are S1–S5.

## Relation to the rest of the repo

| Path | Role |
|---|---|
| `wilibsp/apps/openmicro/` | Synchronized build copy for display firmware |
| `peer/` | Optional/future main-CPU CDC bridge sources |
| Repository root | Portable demo launcher and documentation |

Firmware builds through the vendored Pico BSP. Edit `firmware/openmicro`; the
build/flash wrapper synchronizes it into `wilibsp/apps/openmicro` automatically.

## Manual three-terminal flow

All paths below are inside this checkout:

```powershell
# A — host
cd host
node dist\cli.js --freewili --no-hid claude

# B — RTT
cd wilibsp
py -3 tools\fw.py flash openmicro
py -3 tools\fw.py rtt

# C — bridge
cd host
npm run bridge:rtt
```

Full write-up: [docs/LEGACY-SETUP.md](docs/LEGACY-SETUP.md) and
[docs/openmicro-freewili/README.md](../docs/openmicro-freewili/README.md).
