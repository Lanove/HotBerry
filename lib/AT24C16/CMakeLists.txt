cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(AT24C16)

file(GLOB FILES ./*.cpp ./*.h)
add_library(AT24C16 STATIC ${FILES})

target_link_directories(AT24C16 PRIVATE ../../include)
target_include_directories(AT24C16 PRIVATE ../../include)

# Add the standard library to the build
target_link_libraries(AT24C16 PUBLIC
    pico_stdlib
    hardware_i2c
    FreeRTOS-Kernel-Heap4 # FreeRTOS kernel and dynamic heap
)

target_include_directories(AT24C16 PUBLIC ./)