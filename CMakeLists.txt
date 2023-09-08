cmake_minimum_required(VERSION 3.13)  # CMake version check
project(simple_example)               # Create project "simple_example"
include(CTest)


string(REGEX MATCH ".*/emcc" EMCC_DETECTED "${CMAKE_C_COMPILER}")



if(NOT EMCC_DETECTED)
    add_subdirectory(lib/cpputest)
    if(WIN32)
        add_subdirectory(lib/SDL)
    endif()
    add_subdirectory(tests)
endif()


add_compile_options(-Wall -Wextra -Werror)

add_subdirectory(src)
