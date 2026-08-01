add_executable(store_online
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display/main.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display/store_net.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display/catalog_parse.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display/store_panel.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display/store_ui.c
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display/store_cmd.c)

target_include_directories(store_online PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../../../app-store-online/display
    ${CMAKE_CURRENT_LIST_DIR}/../../../store_uart/include)

# Override CDN at build time, e.g.:
#   -DSTORE_ONLINE_BASE_URL=\"http://192.168.1.10:5173\"
target_compile_definitions(store_online PRIVATE
    "STORE_ONLINE_BASE_URL=\"http://192.168.1.10:5173\"")

target_link_libraries(store_online freewili2_bsp onewili_fwgui)
pico_set_binary_type(store_online copy_to_ram)
pico_add_extra_outputs(store_online)
