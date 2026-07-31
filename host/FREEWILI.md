# FreeWili mode

Drive OpenMicro’s agent host from a FreeWili 2 touchscreen instead of (or
alongside) a DualSense. The PC still owns Claude/Codex; the device sends
high-level **actions** and renders **feedback**.

**This file lives inside the kit copy.** Prefer the kit root:
[`../README.md`](../README.md) (`npm start`). Repo docs index:
[`../../docs/openmicro-freewili/README.md`](../../docs/openmicro-freewili/README.md).

## Board taps → Claude (works today)

Stock main firmware does **not** relay display `a\om\*` wires to USB. Use the
**RTT bridge** so display taps reach OpenMicro over the debug probe:

```powershell
# Terminal A — host
cd "C:\repos\free wili 2\demo_inspo\OpenMicro"
node dist\cli.js --freewili --no-hid claude

# Terminal B — RTT server (display CMSIS-DAP interface 0)
cd "C:\repos\free wili 2\device stuff\wilibsp"
fw flash openmicro
fw rtt

# Terminal C — bridge
cd "C:\repos\free wili 2\demo_inspo\OpenMicro"
npm run bridge:rtt
```

Tap **Accept** on the device. Terminal C should print `OMTX → { type: 'accept' }`
and Claude should see Enter. Terminal A should show `freewili → accept`.

## Quick start (no hardware)

```bash
# Terminal A — host with FreeWili TCP bridge, no gamepad
node dist/cli.js --freewili --no-hid claude

# Terminal B — pretend to be the device
npm run device:sim -- --action prompt --text "Say hi"
```

Spikes:

```bash
npm run spike:windows   # Node version + build + protocol smoke
npm run spike:serial    # COM probe (optional) + TCP round-trip (required)
npm run monitor:com -- --list
```

## Flags

| Flag | Meaning |
|---|---|
| `--freewili` | Enable TCP bridge on `127.0.0.1:48763` |
| `--freewili-tcp [port]` | Custom TCP port (implies `--freewili`) |
| `--freewili-serial COMx` | Also open USB CDC serial JSON (implies `--freewili`) — needs main OpenMicro peer |
| `--no-hid` | Skip gamepad HID |

## Long-term: main OpenMicro peer

Link [`device stuff/main-openmicro-peer`](../../device%20stuff/main-openmicro-peer)
into custom main firmware so CDC carries JSON without the debug probe. Then:

```text
node dist/cli.js --freewili --freewili-serial COMx --no-hid claude
```

## Protocol

Line-delimited JSON (`src/freewili/protocol.ts`). Device → host: `hello`,
`action`, `ping`. Host → device: `hello`, `feedback`, `config`, `pong`.

Display wires: [`docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md`](../../docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md).
Firmware UI: `device stuff/wilibsp/apps/openmicro/`.

```
Touch UI --OMTX/RTT--> bridge:rtt --TCP JSON--> OpenMicro --> Claude
     \--FwGUI a\om\*--> main peer (future) --CDC JSON--/
```

Import the adapter from code via `openmicro/freewili`.
