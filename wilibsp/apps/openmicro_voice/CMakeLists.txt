add_executable(openmicro_voice
    main.c
    voice_wav.c
    voice_ui.c)

target_include_directories(openmicro_voice PRIVATE ${CMAKE_CURRENT_LIST_DIR})
target_link_libraries(openmicro_voice freewili2_bsp)
pico_set_binary_type(openmicro_voice copy_to_ram)
pico_add_extra_outputs(openmicro_voice)
