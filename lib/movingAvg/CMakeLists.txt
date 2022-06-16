cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(movingAvg)

file(GLOB FILES ./*.cpp ./*.h)
add_library(movingAvg ${FILES})

# Add the standard library to the build
target_link_libraries(movingAvg
    pico_stdlib
    )

target_include_directories(movingAvg PUBLIC ./)