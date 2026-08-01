# launcher — persistent flash home shell

Flash-resident (`pico_set_binary_type default`) UI with a centered **APP STORE**
button. Tapping it loads a RAM-only UF2 from USB FatFs, stages it in PSRAM, and
hands off via the RP2350 boot ROM (`REBOOT2_FLAG_REBOOT_TYPE_RAM_IMAGE`).

This follows `AGENTS_FREEWILI_RAM_APP_LOADING.md`, with **USB MSC** in place of
SD (wilibsp has no SD driver today).

## Build / flash

```text
cd "device stuff/wilibsp"
py -3 tools/fw.py build launcher
py -3 tools/fw.py build store_ram
py -3 tools/fw.py flash launcher
```

Do **not** OpenOCD-flash `store_ram` as the resident image — it is `no_flash`
and is meant to be copied to a USB stick.

## USB layout

```text
0:/apps/store_ram.uf2
```

Also accepted:

- `0:/freewili-store/apps/store_ram.uf2`
- `0:/store_ram.uf2`

Copy `build/apps/store_ram/store_ram.uf2` to the stick after building.

## Flow

```text
launcher (flash) → tap APP STORE → validate/stage UF2 → ROM RAM boot
       ↑                                                      │
       └──────── EXIT / reset / power-cycle ←─────────────────┘
```

Launcher and store run **sequentially**, not concurrently.
