# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)
set(PICO_BOARD "hotberry" CACHE STRING [FORCE])

include(pico_sdk_import.cmake)
project(HotBerry C CXX ASM)
message("PICO_BOARD is " ${PICO_BOARD})
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
# Add executable. Default name is the project name, version 0.1

add_executable(HotBerry src/main.cpp )

pico_set_program_name(HotBerry "HotBerry")
pico_set_program_version(HotBerry "0.1")

# no_flash means the target is to run from RAM
pico_set_binary_type(HotBerry default)

pico_enable_stdio_uart(HotBerry 1)
pico_enable_stdio_usb(HotBerry 1)

add_subdirectory(lib/ili9486_drivers)

# Add the standard library to the build
target_link_libraries(HotBerry 
    ili9486_drivers
    pico_stdlib)

# Add any user requested libraries
target_link_libraries(HotBerry 
    pico_bootsel_via_double_reset
    )

pico_add_extra_outputs(HotBerry)