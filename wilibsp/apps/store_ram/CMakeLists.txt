set(STORE_RX ${CMAKE_CURRENT_LIST_DIR}/../store_receiver)

add_executable(store_ram
    main.c
    ${STORE_RX}/catalog_parse.c
    ${STORE_RX}/store_panel.c
    ${STORE_RX}/store_ui.c
    ${STORE_RX}/store_cmd.c)

target_include_directories(store_ram PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${STORE_RX}
    ${CMAKE_CURRENT_LIST_DIR}/../../../store_uart/include)

target_compile_definitions(store_ram PRIVATE STORE_UI_HAS_EXIT=1)

target_link_libraries(store_ram
    freewili2_bsp
    pico_bootrom)

# Temporary RAM-only image — does not survive reset; launcher reloads from USB.
pico_set_binary_type(store_ram no_flash)
pico_add_extra_outputs(store_ram)
