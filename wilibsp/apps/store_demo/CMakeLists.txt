# Reuse parser/UI/cmd from store_receiver (single source of truth).
set(STORE_RX ${CMAKE_CURRENT_LIST_DIR}/../store_receiver)

add_executable(store_demo
    main.c
    ${STORE_RX}/catalog_parse.c
    ${STORE_RX}/store_panel.c
    ${STORE_RX}/store_ui.c
    ${STORE_RX}/store_cmd.c)

target_include_directories(store_demo PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${STORE_RX}
    ${CMAKE_CURRENT_LIST_DIR}/../../../store_uart/include)

target_link_libraries(store_demo freewili2_bsp)
pico_set_binary_type(store_demo copy_to_ram)
pico_add_extra_outputs(store_demo)
