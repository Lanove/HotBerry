cmake_minimum_required(VERSION 3.13)
include(FetchContent)
include(pico_sdk_import.cmake)
include(FreeRTOS_Kernel_import.cmake)
set(PICO_BOARD "hotberry" CACHE STRING [FORCE])
set(LV_CONF_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/include/lv_conf.h
    CACHE STRING "" FORCE)
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

FetchContent_Declare(lvgl GIT_REPOSITORY https://github.com/lvgl/lvgl.git
    GIT_TAG 0b5a1d4b23975b920ff841ea9cd038802f51711b) # Fetch LVGL v8.2
FetchContent_MakeAvailable(lvgl)
project(HotBerry C CXX ASM)

pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
add_executable(HotBerry src/main.cpp)

pico_set_program_name(HotBerry "HotBerry")
pico_set_program_version(HotBerry "0.1")

# no_flash means the target is to run from RAM
pico_set_binary_type(HotBerry default)

pico_enable_stdio_uart(HotBerry 1)
pico_enable_stdio_usb(HotBerry 1)

add_subdirectory(lib/ili9486_drivers)
add_subdirectory(lib/MAX6675)
add_subdirectory(lib/movingAvg)

# Add the standard library to the build
target_link_libraries(HotBerry pico_stdlib)

# Add any user requested libraries
target_link_libraries(HotBerry
    pico_bootsel_via_double_reset
    hardware_clocks
    hardware_adc
    ili9486_drivers
    MAX6675
    lvgl::lvgl
    lvgl::demos
    movingAvg
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
)

pico_add_extra_outputs(HotBerry)