# OneWili C API for WiliBSP (FreeWili 2 display CPU, RP2350B).
#
# This directory is a thin CMake shim. In the portable OpenMicro demo, the
# generated sources are vendored at the demo repository root in `onewili/`.
cmake_minimum_required(VERSION 3.20)

set(ONEWILI_SRC_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../../onewili")
if(NOT EXISTS "${ONEWILI_SRC_ROOT}/CMakeLists.txt")
    message(FATAL_ERROR
        "OneWili sources not found at ${ONEWILI_SRC_ROOT}. "
        "Clone/copy the generated onewili/wilibsp tree next to wilibsp, "
        "or replace this shim with a full vendored copy under libs/onewili.")
endif()

add_library(onewili_fwgui STATIC
    ${ONEWILI_SRC_ROOT}/src/onewili.c
    ${ONEWILI_SRC_ROOT}/src/onewili_events.c
    ${ONEWILI_SRC_ROOT}/src/binary_framing.c
    ${ONEWILI_SRC_ROOT}/src/binary_transport.c
    ${ONEWILI_SRC_ROOT}/src/onewili_fwgui.c
)
target_include_directories(onewili_fwgui PUBLIC ${ONEWILI_SRC_ROOT}/include)
target_link_libraries(onewili_fwgui PUBLIC pico_stdlib hardware_uart hardware_gpio)
