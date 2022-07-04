cmake_minimum_required(VERSION 3.13)
include(pico_sdk_import.cmake)
set(PICO_BOARD "hotberry" CACHE STRING [FORCE])
set(LV_CONF_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h
    CACHE STRING "" FORCE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

add_subdirectory(/home/kohigashi/Projects/lvgl ./build)
project(HotBerry C CXX ASM)

pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(HotBerry
    src/main.cpp
    src/lv_drivers.cpp
)

pico_set_program_name(HotBerry "HotBerry")
pico_set_program_version(HotBerry "0.1")

# no_flash means the target is to run from RAM
pico_set_binary_type(HotBerry default)

pico_enable_stdio_uart(HotBerry 1)
pico_enable_stdio_usb(HotBerry 1)

add_subdirectory(lib/ili9486_drivers)
add_subdirectory(lib/MAX6675)
add_subdirectory(lib/movingAvg)
add_subdirectory(lib/lv_app)
add_subdirectory(lib/AT24C16)

target_link_directories(HotBerry PRIVATE ./include)

target_link_libraries(HotBerry PRIVATE
    pico_stdlib
    pico_bootsel_via_double_reset
    hardware_clocks
    hardware_adc
    hardware_i2c
    ili9486_drivers
    MAX6675
    AT24C16
    lv_app
    lvgl
    lvgl::lvgl
    lvgl::demos
    movingAvg
)

pico_add_extra_outputs(HotBerry)