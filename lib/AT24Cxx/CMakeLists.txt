cmake_minimum_required(VERSION 3.13)

include(../../pico_sdk_import.cmake)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()
project(AT24Cxx)

file(GLOB FILES ./*.cpp ./*.h)
add_library(AT24Cxx ${FILES})

# Add the standard library to the build
target_link_libraries(AT24Cxx
    pico_stdlib
    )

target_include_directories(AT24Cxx PUBLIC ./)