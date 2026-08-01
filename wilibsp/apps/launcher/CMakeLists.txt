add_executable(launcher main.c)

target_include_directories(launcher PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../services/app_loader)

target_link_libraries(launcher
    freewili2_bsp
    app_loader)

# Persistent flash/XIP resident shell — must NOT be copy_to_ram so SRAM
# remains free for the staged no_flash app at handoff time.
pico_set_binary_type(launcher default)
pico_add_extra_outputs(launcher)
