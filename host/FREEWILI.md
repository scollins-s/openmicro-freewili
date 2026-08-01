# FreeWili mode

Drive OpenMicro’s agent host from a FreeWili 2 touchscreen instead of (or
alongside) a DualSense. The PC still owns Claude/Codex; the device sends
high-level **actions** and renders **feedback**.

Prefer the repository root [`../README.md`](../README.md) and `npm start` for
the portable one-terminal demo.

## Board taps → Claude (works today)

Stock main firmware does **not** relay display `a\om\*` wires to USB. Use the
**RTT bridge** so display taps reach OpenMicro over the debug probe:

```powershell
# Terminal A — host (from repository root)
cd host
node dist\cli.js --freewili --no-hid claude

# Terminal B — RTT server (display CMSIS-DAP interface 0)
cd wilibsp
py -3 tools\fw.py flash openmicro
py -3 tools\fw.py rtt

# Terminal C — bridge
cd host
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

Link the repository's [`peer/`](../peer/) sources into custom main firmware so
CDC carries JSON without the debug probe. Then:

```text
node dist/cli.js --freewili --freewili-serial COMx --no-hid claude
```

## Protocol

Line-delimited JSON (`src/freewili/protocol.ts`). Device → host: `hello`,
`action`, `ping`. Host → device: `hello`, `feedback`, `config`, `pong`.

Firmware UI source: [`../firmware/openmicro`](../firmware/openmicro/).

```
Touch UI --OMTX/RTT--> bridge:rtt --TCP JSON--> OpenMicro --> Claude
     \--FwGUI a\om\*--> main peer (future) --CDC JSON--/
```

Import the adapter from code via `openmicro/freewili`.
