cmake_minimum_required(VERSION 3.14)
project(snakefish)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_VERBOSE_MAKEFILE on)

find_package(Python3 COMPONENTS Development)

execute_process(COMMAND python3-config --extension-suffix
        OUTPUT_VARIABLE LIB_SUFFIX
        OUTPUT_STRIP_TRAILING_WHITESPACE)

add_library(snakefish SHARED
        src/channel.cpp
        src/channel.h
        src/snakefish.cpp
        src/snakefish.h
        src/thread.cpp
        src/thread.h)

target_include_directories(snakefish PRIVATE
        include
        ${Python3_INCLUDE_DIRS})

target_compile_options(snakefish PRIVATE
        -Wall
        -Wextra)

target_link_options(snakefish PRIVATE
        -Wl,-undefined,dynamic_lookup)

set_target_properties(snakefish PROPERTIES PREFIX "c")
set_target_properties(snakefish PROPERTIES SUFFIX ${LIB_SUFFIX}) # switch to Python3_SOABI when cmake 3.17 is available
