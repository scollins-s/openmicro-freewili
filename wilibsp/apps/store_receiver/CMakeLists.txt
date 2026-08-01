add_executable(store_receiver
    main.c
    catalog_parse.c
    store_panel.c
    store_ui.c
    store_cmd.c)

target_include_directories(store_receiver PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../../store_uart/include)

target_link_libraries(store_receiver freewili2_bsp)
pico_set_binary_type(store_receiver copy_to_ram)
pico_add_extra_outputs(store_receiver)
