cmake_minimum_required(VERSION 3.13)

if(PICO_BOARD)
    include(../../pico_sdk_import.cmake)
    pico_sdk_init()
endif()

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

project(lv_app)

file(GLOB FILES ./*.cpp ./*.c ./*.h)
add_library(lv_app STATIC ${FILES})
target_include_directories(lv_app PUBLIC ./)
target_include_directories(lv_app PUBLIC ../AT24C16)

target_link_libraries(lv_app PRIVATE lvgl)

if(PICO_BOARD)
    target_link_libraries(lv_app PRIVATE
    AT24C16)
    target_link_libraries(lv_app PUBLIC 
    pico_multicore
    pico_sync)
endif()