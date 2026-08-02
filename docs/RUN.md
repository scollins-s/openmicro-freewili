# How to run

## Recommended: one terminal (this kit)

After [KIT-SETUP.md](KIT-SETUP.md):

```powershell
cd <path-to-your-clone>
npm start
```

The launcher starts three child processes in **one** window:

1. OpenOCD RTT server on `127.0.0.1:9090` (via wilibsp `fw.rtt_command`)
2. OpenMicro host: `--freewili --no-hid claude`
3. RTT → TCP bridge (`host/scripts/rtt-bridge.ts`)

The OpenMicro host owns the terminal, so normal startup shows only the
Claude/Codex UI. Add `--show-rtt` to show prefixed OpenOCD and bridge diagnostics.
Stop with **Ctrl+C** (or `npm run stop` if something was left running).

### Useful flags

```powershell
npm start -- --agent codex
npm start -- --cwd C:\work\another-project # agent starts here
npm start -- --show-rtt             # infrastructure and action logs
npm start -- --iface 1              # CMSIS-DAP interface
npm start -- --no-rtt               # host only (you run OpenOCD or use CDC)
npm start -- --skip-bridge          # OpenOCD + host, no bridge
npm run freewili:sim                # host + device:sim, no probe
```

`--project` is an alias for `--cwd`. `OPENMICRO_CWD` supplies the default when
neither flag is present. Relative paths are resolved from the directory where
you invoke `npm start`.

### Expected signal path

1. Flash display once (`npm run flash:display`).
2. `npm start` — wait for the agent UI (`--show-rtt` displays readiness logs).
3. Tap **Accept** on the FreeWili.
4. `[bridge] OMTX → { type: 'accept' }` and host `freewili → accept`.

## Manual: three processes (same kit `host/`)

Useful for debugging one piece at a time:

```powershell
# Terminal A
cd host
node dist\cli.js --freewili --no-hid claude

# Terminal B
cd wilibsp
py -3 tools\fw.py rtt

# Terminal C
cd host
npm run bridge:rtt
```

Note: `fw rtt` also opens its own RTT client for console streaming. The kit
launcher starts OpenOCD **without** that consumer so the bridge can own `:9090`.

## No hardware

```powershell
npm run freewili:sim
```

Or manually:

```powershell
# A
node host\dist\cli.js --freewili --no-hid claude
# B
cd host
npm run device:sim -- --action prompt --text "Say hi"
```

## Future: CDC path

Requires linking `peer/` into custom **main** firmware (see `peer/INTEGRATION.md`).
Then a single host process is enough:

```powershell
node host\dist\cli.js --freewili --freewili-serial COMx --no-hid claude
```

No OpenOCD, no RTT bridge. Stock main does not support this yet.

## Troubleshooting

| Symptom | Check |
|---|---|
| `Host not built` | `npm run setup` |
| RTT wait timeout | Probe plugged in; try `--iface 1`; nothing else holding SWD |
| Bridge cannot connect to OpenMicro | Host must be up with `--freewili` |
| Bridge cannot connect to RTT | OpenOCD running; kit launcher preferred over `fw rtt` + bridge together |
| Ports stuck | `npm run stop` |
