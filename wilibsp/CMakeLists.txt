cmake_minimum_required(VERSION 3.13)

# RP2350B board header (PICO_RP2350A=0 -> 48 GPIO, 16MB flash).
# Do NOT pass -DPICO_BOARD on the command line — that overrides this.
set(PICO_BOARD freewili2 CACHE STRING "Board type")
list(APPEND PICO_BOARD_HEADER_DIRS "${CMAKE_CURRENT_LIST_DIR}/bsp/boards")

# Prefer prebuilt pioasm/picotool from the Pico VS Code ~/.pico-sdk install
# (avoids needing a host MSVC/MinGW toolchain to build those tools from source).
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if(EXISTS ${picoVscode})
    include(${picoVscode})
endif()

include(pico_sdk_import.cmake)
project(wilibsp C CXX ASM)
set(CMAKE_C_STANDARD 11)
pico_sdk_init()

add_subdirectory(bsp)

# Reusable non-BSP libraries.
add_subdirectory(libs/onewili)

# Services (UF2 → PSRAM stage → ROM RAM handoff).
add_subdirectory(services/app_loader)

# Each app opts in; add new apps here.
add_subdirectory(apps/hello_audio)
add_subdirectory(apps/hello_cc1101)
add_subdirectory(apps/hello_display)
add_subdirectory(apps/hello_ir)
add_subdirectory(apps/hello_mics)
add_subdirectory(apps/hello_sensors)
add_subdirectory(apps/hello_usbdrive)
add_subdirectory(apps/hello_dvi)
add_subdirectory(apps/hello_buttons)
add_subdirectory(apps/toggleled)
set(STORE_UART_ROOT "${CMAKE_CURRENT_LIST_DIR}/../store_uart")
if(EXISTS "${STORE_UART_ROOT}/include/store_uart_proto.h")
    add_subdirectory(apps/store_receiver)
    add_subdirectory(apps/store_demo)
    add_subdirectory(apps/store_ram)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../app-store-online/display/main.c")
        add_subdirectory(apps/store_online)
    else()
        message(STATUS "Skipping store_online (optional app-store-online sources not found)")
    endif()
else()
    message(STATUS "Skipping optional store apps (store_uart sources not found)")
endif()
add_subdirectory(apps/launcher)
add_subdirectory(apps/openmicro)
add_subdirectory(apps/openmicro_voice)
add_subdirectory(apps/http_viewer)
