# store_ram — RAM-only FreeWili app store

`no_flash` build of the display app store. Same USB catalog UI as
`store_receiver`, plus an **EXIT** button that performs a normal reboot back to
the persistent `launcher` in flash.

## Build

```text
py -3 tools/fw.py build store_ram
```

Output: `build/apps/store_ram/store_ram.uf2` — copy to USB as `apps/store_ram.uf2`.

## Close / return

- Tap **EXIT** (header, left of OFF) → `rom_reboot(NORMAL)` → flash launcher
- Hardware reset / power cycle → same

Do not attempt to reconstruct the launcher in SRAM from this app.
