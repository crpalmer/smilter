set(PICO_BOARD pico_w)

cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

include(pico_sdk_import.cmake)

project(smilter)

pico_sdk_init()

add_definitions(-DPI_PICO)
add_compile_definitions(CYW43_HOST_NAME=\"smilter\")

set(MAKE_FS_DATA_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/external/makefsdata)

#
# To regenerate the fs data you need to rerun cmake ..
#

message("Running makefsdata script")
execute_process(COMMAND
        perl ${MAKE_FS_DATA_SCRIPT}
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        ECHO_OUTPUT_VARIABLE
        ECHO_ERROR_VARIABLE
        )
file(RENAME fsdata.c fsdata_custom.c)

include_directories(../../lib .)
link_directories(../../lib/build)

add_executable(smilter
        smilter.cpp
        )

# pull in common dependencies
target_link_libraries(smilter
    pi
    pico_cyw43_arch_threadsafe_background
    pico_cyw43_arch_lwip_threadsafe_background
    pico_lwip_http
    pico_stdlib
    pico_multicore
    hardware_adc
    hardware_gpio
    hardware_pio
)

# create map/bin/hex file etc.
pico_add_extra_outputs(smilter)

# enable usb output, disable uart output
pico_enable_stdio_usb(smilter 1)
pico_enable_stdio_uart(smilter 0)
