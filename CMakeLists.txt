cmake_minimum_required(VERSION 3.16)

project(ubx-gnss-c VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_library(ubx
    src/ubx_checksum.c
    src/ubx_frame.c
    src/ubx_parser.c
    src/ubx_nav_pvt.c
    src/ubx_ack.c
)

target_include_directories(ubx
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
    target_compile_options(ubx PRIVATE /W4)
else()
    target_compile_options(ubx PRIVATE -Wall -Wextra -Wpedantic)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
