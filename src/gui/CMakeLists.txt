if(EMCC_DETECTED)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()



set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "output")

if(EMCC_DETECTED)
    add_executable(index main.c)
    target_link_libraries(index PRIVATE chip8)
    target_link_options(index PRIVATE -s USE_SDL=2 --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/template.html" -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=_jscallback,_main,_malloc,_free)
    # --embed-file ${CMAKE_SOURCE_DIR}/assets@assets --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/template.html"
    target_compile_options(index PRIVATE -s USE_SDL=2)

elseif(WIN32)
    add_executable(index main.c)
    target_link_libraries(index PRIVATE chip8 mingw32 SDL2main SDL2)
    # ToDo: copy SDL2d.dll to build folder
endif()

