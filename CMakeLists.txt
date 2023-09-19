cmake_minimum_required(VERSION 3.13)  # CMake version check
project(simple_example)               # Create project "simple_example"
include(CTest)


string(REGEX MATCH ".*/emcc" EMCC_DETECTED "${CMAKE_C_COMPILER}")



if(NOT EMCC_DETECTED)
    add_subdirectory(lib/cpputest)
    add_subdirectory(lib/SDL)
    add_subdirectory(tests)
endif()


add_compile_options(-Wall -Wextra -Werror -Wwrite-strings -Wno-error=unused-variable -Wno-error=unused-parameter -Wno-error=unused-function -Wno-error=unused-but-set-variable)

add_subdirectory(src)
