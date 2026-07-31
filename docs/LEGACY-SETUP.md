# Legacy multi-folder setup

The FreeWili OpenMicro stack originally lived in several places in this repo.
That layout still works and is left in place. Prefer
[`openmicro-freewili/`](../README.md) for day-to-day bring-up; use this page
when working on the originals or comparing paths.

## Folder map

| Path | Role |
|---|---|
| `demo_inspo/OpenMicro/` | PC OpenMicro host + FreeWili TCP/serial + `bridge:rtt` |
| `device stuff/wilibsp/apps/openmicro/` | Display touch firmware (build/flash home) |
| `device stuff/wilibsp/` | BSP + `fw build` / `fw flash` / `fw rtt` |
| `device stuff/main-openmicro-peer/` | Main CDC peer (future) |
| `docs/openmicro-freewili/` | Protocol / design docs |

## Three-terminal run (original)

```powershell
# Terminal A — host
cd "C:\repos\free wili 2\demo_inspo\OpenMicro"
npm install
npm run build
node dist\cli.js --freewili --no-hid claude

# Terminal B — RTT (after flashing once)
cd "C:\repos\free wili 2\device stuff\wilibsp"
fw flash openmicro
fw rtt

# Terminal C — bridge
cd "C:\repos\free wili 2\demo_inspo\OpenMicro"
npm run bridge:rtt
```

Authoritative short form: [`demo_inspo/OpenMicro/FREEWILI.md`](../../demo_inspo/OpenMicro/FREEWILI.md).

Repo index for both kits: [`docs/openmicro-freewili/README.md`](../../docs/openmicro-freewili/README.md).

## Why three prompts

1. **Host** owns the agent PTY and FreeWili TCP listen port.
2. **OpenOCD** owns the debug probe and RTT TCP server (`:9090`).
3. **Bridge** is a separate Node process that was added without changing the
   host CLI — it adapts RTT `OMTX` lines into FreeWili JSON actions.

The kit launcher (`scripts/start.mjs`) runs all three as children of one
command. A future `--freewili-serial` + main peer path can drop (2) and (3).
