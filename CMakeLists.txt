cmake_minimum_required(VERSION 3.13)  # CMake version check
project(simple_example)               # Create project "simple_example"
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "output")

string(REGEX MATCH ".*/emcc" EMCC_DETECTED "${CMAKE_C_COMPILER}")
if(EMCC_DETECTED)
else()
    message( FATAL_ERROR "Must run with emcmake" )
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -DTEST_DEFINE")

add_library(xlib STATIC src/x.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(index.html src/main.c)
target_link_libraries(index.html xlib)

add_subdirectory(tests)
