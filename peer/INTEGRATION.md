# Integrating OpenMicro peer into FreeWili 2 main firmware

## Prerequisites

1. Access to FreeWili main firmware sources (not shipped in this repo).
2. Display running `wilibsp/apps/openmicro` (sends `a\om\*`).
3. PC OpenMicro with `--freewili --freewili-serial COMx`.

## Checklist

1. Add `om_cmd_bridge.c` / `.h` to the main build.
2. `om_cmd_bridge_init(&b, emit_fwgui_event, write_cdc_line, user)`.
3. In the FwGUI quiet-path handler (same place as store/http peers):

```c
if (om_cmd_bridge_handle_display_line(&om_bridge, line)) {
    /* consumed */
}
```

4. In the USB CDC RX path, accumulate lines and:

```c
om_cmd_bridge_handle_cdc_line(&om_bridge, line);
```

5. `emit` must format spontaneous OneWili text events toward display:

```text
[*omFb {…json…}]
[*omCfg {…json…}]
[*omHello host]
```

6. Verify with `npm run monitor:com -- COMx` — taps should show JSON
   `{"v":1,"type":"action",…}`.

## Fallback without main sources

Use `npm run bridge:rtt` while `fw rtt` is attached to the display CPU.
