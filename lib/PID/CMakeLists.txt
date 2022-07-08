cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(PID)

file(GLOB FILES ./*.cpp ./*.h)
add_library(PID STATIC ${FILES})

# Add the standard library to the build
target_link_libraries(PID PUBLIC
    pico_stdlib
    pico_platform
    hardware_i2c
    )

target_include_directories(PID PUBLIC ./)