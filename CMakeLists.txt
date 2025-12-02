cmake_minimum_required(VERSION 3.18)
project(aoc-2025)
set(FEATURE_MACROS -D_POSIX_C_SOURCE=200809L)
file(GLOB SOURCES "*.c")

foreach(SOURCE_FILE ${SOURCES})

    # Get the file name without the extension
    get_filename_component(EXECUTABLE_NAME ${SOURCE_FILE} NAME_WE)
    
    # Add an executable for each source file
    add_executable(${EXECUTABLE_NAME} ${SOURCE_FILE})
    target_compile_definitions(${EXECUTABLE_NAME} PUBLIC ${FEATURE_MACROS})

endforeach()

if (CMAKE_C_COMPILER_ID MATCHES Clang OR ${CMAKE_C_COMPILER_ID} STREQUAL GNU)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -pedantic -g -Og")
endif ()
