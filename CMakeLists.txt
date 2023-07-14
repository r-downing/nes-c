cmake_minimum_required(VERSION 3.13)  # CMake version check
project(simple_example)               # Create project "simple_example"


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "output")

string(REGEX MATCH ".*/emcc" EMCC_DETECTED "${CMAKE_C_COMPILER}")
if(EMCC_DETECTED)
else()
    message( FATAL_ERROR "Must run with emcmake" )
endif()

include_directories(inc)

add_compile_options(-Wall -Wextra -DTEST_DEFINE)

add_library(chip8 STATIC src/chip8.c src/chip8_font_set.c src/chip8_impl.c)

set(CMAKE_EXECUTABLE_SUFFIX ".html")

add_executable(index src/main.c)
target_link_libraries(index chip8)

add_subdirectory(tests)
