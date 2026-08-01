add_executable(http_viewer
    main.c
    http_net.c
    response_store.c
    view.c
)

target_include_directories(http_viewer PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(http_viewer PRIVATE freewili2_bsp onewili_fwgui)
pico_set_binary_type(http_viewer copy_to_ram)
pico_add_extra_outputs(http_viewer)
