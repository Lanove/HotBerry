cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(AT24C16)

file(GLOB FILES ./*.cpp ./*.h)
add_library(AT24C16 ${FILES})

# Add the standard library to the build
target_link_libraries(AT24C16 PRIVATE
    pico_stdlib
    hardware_i2c
    )

target_include_directories(AT24C16 PUBLIC ./)