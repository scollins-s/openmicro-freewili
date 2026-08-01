# store_receiver (display store UI)



FreeWili 2 **display-CPU** app: browse `0:/freewili-store/catalog.json` on a USB

stick (touch UI + RTT commands).



Full build / load / test guide:

[`docs/bottlenose-store-agent/BUILD-AND-TEST.md`](../../../../docs/bottlenose-store-agent/BUILD-AND-TEST.md).



## Scope



| Does | Does not |

|---|---|

| Mount USB MSC, parse catalog, list/detail/help UI | Talk Wi‑Fi UART / FWSA (main owns onboard ESP) |

| RTT: `help` `list` `refresh` `get <id>` | Flash UF2 / run wasm |

| Offline MVP | Online CDN until OneWili store cmds + main peer exist |



Catalog JSON: accepts the **web** schema (`catalog_version` +

`targets.displaycpu|maincpu|esp32`) and the docs `artifacts[]` schema.



## Build / flash / RTT



```text

cd "device stuff/wilibsp"

py -3 tools/fw.py build store_receiver

py -3 tools/fw.py flash store_receiver

py -3 tools/fw.py rtt

```



OpenOCD **interface 0** (display). Main CPU should keep stock firmware if you

also use OneWili apps (`toggleled`).



Requires Pico SDK + CMake + Ninja (see AGENTS.md).



## Apply / run



[`docs/bottlenose-store-agent/RUN-DESTINATIONS.md`](../../../../docs/bottlenose-store-agent/RUN-DESTINATIONS.md).

