# app_loader service

Implements the SD/USB → PSRAM → ROM RAM-image path from
`AGENTS_FREEWILI_RAM_APP_LOADING.md` for FreeWili 2 display (RP2350B).

| Piece | Role |
|---|---|
| `uf2_reader.*` | Host-testable UF2 parse + SRAM allowlist validation |
| `app_loader.*` | FatFs read, PSRAM stage, public API |
| `handoff.*` | Quiesce IRQs/DMA, PSRAM stack, final copy, `rom_reboot` |

## Media note

The design guide names SD. This BSP exposes **USB MSC FatFs** (`0:/`) only; the
launcher looks under `0:/apps/…`.

## Handoff contract

1. Validate every UF2 block (magic, family, SRAM-only targets, completeness).
2. Stage payloads + metadata in PSRAM (never overwrite running SRAM early).
3. Disable IRQs, switch SP to PSRAM, copy into SRAM targets.
4. `rom_reboot(REBOOT2_FLAG_REBOOT_TYPE_RAM_IMAGE | …)`.

Return path is a **normal reboot** from the RAM app (see `apps/store_ram`).
