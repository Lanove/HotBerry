cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

project(lv_app)

file(GLOB FILES ./*.cpp ./*.c ./*.h)
add_library(lv_app ${FILES})
target_include_directories(lv_app PUBLIC ./)

target_link_libraries(lv_app PRIVATE
    lvgl
    lvgl::examples
    lvgl::demos)

if(PICO_BOARD)
    message("LV_APP USES EEPROM")
    target_link_libraries(lv_app PRIVATE AT24C16)
endif()