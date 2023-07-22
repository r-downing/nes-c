cmake_minimum_required(VERSION 3.13)  # CMake version check
project(simple_example)               # Create project "simple_example"


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "output")

string(REGEX MATCH ".*/emcc" EMCC_DETECTED "${CMAKE_C_COMPILER}")


add_compile_options(-Wall -Wextra)

add_subdirectory(src)


if(EMCC_DETECTED)

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
    add_executable(index src/main.c)
    target_link_libraries(index chip8)
    target_link_options(index PRIVATE -s USE_SDL=2 --preload-file ../assets@assets)
    target_compile_options(index PRIVATE -s USE_SDL=2)
else()

    add_subdirectory(lib/cpputest)
    add_subdirectory(tests)

endif()
