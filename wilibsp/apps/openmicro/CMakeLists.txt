add_executable(openmicro
    main.c
    om_ui.c
    om_link.c
    om_leds.c)

target_include_directories(openmicro PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(openmicro freewili2_bsp onewili_fwgui)
pico_set_binary_type(openmicro copy_to_ram)
pico_add_extra_outputs(openmicro)
