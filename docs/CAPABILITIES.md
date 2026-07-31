# Capabilities

FreeWili OpenMicro turns the FreeWili 2 **display** into a touch remote for
OpenMicro’s agent host on the PC. The PC still owns Claude/Codex; the device
sends high-level **actions** and can receive **feedback**.

## What works today (RTT MVP)

- Custom 480×320 touch UI: Accept, Reject, Voice, New, workflows, session chips,
  thinking depth, layer, mini d-pad.
- Display logs `OMTX a\om\*` on SEGGER RTT and also sends FwGUI wires toward main.
- PC OpenMicro listens on TCP `127.0.0.1:48763` (`--freewili`).
- `bridge:rtt` (or this kit’s launcher) converts RTT `OMTX` lines into JSON
  actions and injects them into the host.
- No custom **main** firmware required for board → Claude.
- No-hardware bring-up via `device:sim` / `npm run freewili:sim`.

## What does not work yet without extra main work

- Stock main does **not** relay `a\om\*` to USB CDC.
- Full closed-loop display feedback (host → main → UI) needs the main OpenMicro
  peer linked into custom main firmware (see `peer/` and
  `docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md`).

## Future: USB CDC (one PC process, no probe)

After linking `peer/om_cmd_bridge.*` into main:

```text
node host/dist/cli.js --freewili --freewili-serial COMx --no-hid claude
```

That path drops OpenOCD and the RTT bridge. Documented in
[RUN.md](RUN.md#future-cdc-path).

## Protocol sketch

```text
Touch UI --OMTX/RTT--> OpenOCD :9090 --> bridge --> TCP JSON :48763 --> OpenMicro --> Claude
     \--FwGUI a\om\*--> main peer (future) --CDC JSON------------------------------/
```

Wire paths: [`docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md`](../../docs/openmicro-freewili/ONEWILI-OPENMICRO-COMMANDS.md).
