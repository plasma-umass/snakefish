# preamble
cmake_minimum_required(VERSION 3.14)
project(snakefish)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on)

# snakefish
find_package(Python3 COMPONENTS Development)

execute_process(COMMAND python3-config --extension-suffix
        OUTPUT_VARIABLE LIB_SUFFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(snakefish SHARED
        src/buffer.cpp
        src/buffer.h
        src/channel.cpp
        src/channel.h
        src/shared_buffer.cpp
        src/shared_buffer.h
        src/snakefish.cpp
        src/snakefish.h
        src/thread.cpp
        src/thread.h
        src/util.h)

target_include_directories(snakefish PRIVATE
        include
        ${Python3_INCLUDE_DIRS})

if(${CMAKE_SYSTEM_NAME} STREQUAL Linux)
    target_link_libraries(snakefish PRIVATE atomic)
endif()

target_compile_options(snakefish PRIVATE
        -Wall
        -Wextra)

target_link_options(snakefish PRIVATE
        -Wl,-undefined,dynamic_lookup)

set_target_properties(snakefish PROPERTIES PREFIX "c")
set_target_properties(snakefish PROPERTIES SUFFIX ${LIB_SUFFIX}) # switch to Python3_SOABI when cmake 3.17 is available

# test
add_subdirectory(pybind11)
add_subdirectory(googletest)

add_executable(test
        src/tests/main.cpp
        src/tests/channel_tests.h
        src/tests/shared_buffer_tests.h
        src/tests/test_util.h)

target_include_directories(test PRIVATE
        src
        pybind11/include
        googletest/googletest/include
        ${Python3_INCLUDE_DIRS})

target_link_libraries(test PRIVATE
        snakefish
        pybind11::embed
        gtest)

target_compile_options(test PRIVATE
        -Wall
        -Wextra)
