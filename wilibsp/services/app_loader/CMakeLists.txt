# App loader — validate UF2, stage in PSRAM, ROM RAM-image handoff
add_library(app_loader STATIC
    uf2_reader.c
    app_loader.c
    handoff.c)

target_include_directories(app_loader PUBLIC
    ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(app_loader PUBLIC
    freewili2_bsp
    pico_bootrom
    hardware_irq
    hardware_sync)
