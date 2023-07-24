if(NOT EMCC_DETECTED)
    return()
endif()

set(CMAKE_EXECUTABLE_SUFFIX ".html")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "output")

add_executable(index main.c)
target_link_libraries(index chip8)
target_link_options(index PRIVATE -s USE_SDL=2 --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/template.html" -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=_jscallback,_main,_malloc,_free)
# --embed-file ${CMAKE_SOURCE_DIR}/assets@assets
target_compile_options(index PRIVATE -s USE_SDL=2)
