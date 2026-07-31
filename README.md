# OpenMicro FreeWili kit

One-folder setup for driving **OpenMicro** (Claude/Codex on the PC) from a
**FreeWili 2** touchscreen.

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
  host/                 OpenMicro PC host (copy of demo_inspo/OpenMicro)
  firmware/openmicro/   Display app sources (snapshot; build via wilibsp)
  peer/                 Main-CPU CDC bridge kit (optional / future)
  scripts/start.mjs     One-command: OpenOCD + host + RTT bridge
  scripts/flash-display.ps1
  docs/                 Setup and run guides
```

## Quick start

**Prereqs:** Node.js ≥ 22, Python 3 (`py`), Pico SDK / OpenOCD (as used by
`device stuff/wilibsp`), Claude or Codex CLI on `PATH`, FreeWili 2 + CMSIS-DAP.

```powershell
cd "C:\repos\free wili 2\openmicro-freewili"
npm run setup
npm run flash:display          # once — probe connected, display CPU
npm start                      # ONE terminal: OpenOCD + host + bridge
```

Tap **Accept** on the device. You should see `[bridge] OMTX → …` and the host
log `freewili → accept`.

No hardware:

```powershell
npm run freewili:sim
```

More: [docs/KIT-SETUP.md](docs/KIT-SETUP.md) · [docs/RUN.md](docs/RUN.md).

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
npm start -- --iface 1
npm start -- --no-rtt          # host only
npm start -- --sim
```

## Relation to the rest of the repo

| Path | Role |
|---|---|
| `demo_inspo/OpenMicro/` | Original PC host (still valid) |
| `device stuff/wilibsp/apps/openmicro/` | Build/flash home for display firmware |
| `device stuff/main-openmicro-peer/` | Original main peer sources |
| **`openmicro-freewili/`** | This kit — copies + one-command run |

Firmware still **builds through wilibsp** (Pico BSP). Edit either the kit
snapshot or the wilibsp app; use `npm run sync:firmware` before flash if you
changed the kit copy.

## Legacy three-terminal flow

If you prefer the original paths without this kit:

```powershell
# A — host
cd "C:\repos\free wili 2\demo_inspo\OpenMicro"
node dist\cli.js --freewili --no-hid claude

# B — RTT
cd "C:\repos\free wili 2\device stuff\wilibsp"
fw flash openmicro
fw rtt

# C — bridge
cd "C:\repos\free wili 2\demo_inspo\OpenMicro"
npm run bridge:rtt
```

Full write-up: [docs/LEGACY-SETUP.md](docs/LEGACY-SETUP.md) and
[docs/openmicro-freewili/README.md](../docs/openmicro-freewili/README.md).
