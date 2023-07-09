cmake_minimum_required(VERSION 3.13)  # CMake version check
set(CMAKE_C_COMPILER "emcc")
set(CMAKE_CXX_COMPILER "emcc")
project(simple_example)               # Create project "simple_example"
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -DTEST_DEFINE")

# Add main.cpp file of project root directory as source file
set(SOURCE_FILES main.c x.c)

# Add executable target with source files listed in SOURCE_FILES variable
add_executable(main.js ${SOURCE_FILES})
