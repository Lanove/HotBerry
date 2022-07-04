cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(ili9486_drivers)

file(GLOB FILES ./*.cpp ./*.h)
add_library(ili9486_drivers ${FILES})
pico_generate_pio_header(ili9486_drivers ${CMAKE_CURRENT_LIST_DIR}/pio_8bit_parallel.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(ili9486_drivers PRIVATE
    pico_stdlib
    hardware_adc
    hardware_irq
    )
# Following two libraries must be PUBLIC, idk
target_link_libraries(ili9486_drivers PUBLIC 
    hardware_pio
    hardware_dma
)

target_include_directories(ili9486_drivers PUBLIC ./)