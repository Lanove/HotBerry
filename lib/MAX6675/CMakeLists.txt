cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(MAX6675)

file(GLOB FILES ./*.cpp ./*.h)
add_library(MAX6675 STATIC ${FILES})

# Add the standard library to the build
target_link_libraries(MAX6675 PRIVATE
    pico_stdlib
    )

target_include_directories(MAX6675 PUBLIC ./)