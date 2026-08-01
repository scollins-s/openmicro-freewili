# OpenMicro peer (main CPU)

Bridge FreeWili **display** taps to the PC OpenMicro host over main USB CDC.

```text
Display openmicro app --FwGUI a\om\*--> MAIN (this peer) --JSON CDC--> OpenMicro --freewili-serial
```

This folder contains the optional main-CPU peer sources. Day-to-day RTT
bring-up (no main flash) uses the repository launcher — see
[`../README.md`](../README.md).

## Immediate path (no main flash): RTT bridge

Until this peer is linked into main firmware, use the **RTT bridge** (works today):

```text
1. npm run flash:display       # display
2. npm start                   # host + OpenOCD RTT + bridge
```

Display logs `OMTX a\om\…`; the bridge injects JSON actions into OpenMicro TCP.

## Wire-up (custom main)

1. Link `om_cmd_bridge.c` into FreeWili **main** firmware.
2. On each FwGUI quiet-path line from the display, call
   `om_cmd_bridge_handle_display_line()`.
3. On each USB CDC RX line from the PC, call
   `om_cmd_bridge_handle_cdc_line()`.
4. Callbacks:
   - `cdc_write` → write one `\n`-terminated JSON line on the PC text CDC port
   - `emit` → send `[*id args]\n` on FwGUI toward the display (`omFb` / `omCfg` / `omHello`)
5. PC: `node dist/cli.js --freewili --freewili-serial COMx --no-hid claude`

Host unit test: `py -3 tools/host_om_cmd_bridge_test.py` (needs `gcc`).

## Do not

- Speak FWSA over FwGUI.
- Expect stock main to understand `a\om\*` without this peer.
