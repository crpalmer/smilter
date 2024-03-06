set(PICO_BOARD pico_w)

cmake_minimum_required(VERSION 3.12)

# Pull in SDK (must be before project)

include(pico_sdk_import.cmake)

project(smilter)

pico_sdk_init()

add_definitions(-DPI_PICO)

include_directories(../../lib)
link_directories(../../lib/build)

add_executable(smilter
        smilter.cpp
        )

# pull in common dependencies
target_link_libraries(smilter
    pi
    pico_cyw43_arch_threadsafe_background
    pico_cyw43_arch_lwip_threadsafe_background 
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
