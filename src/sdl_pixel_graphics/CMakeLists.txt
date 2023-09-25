

if(EMCC_DETECTED)
    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# add_library(sdl_pixel_graphics STATIC sdl_pixel_graphics.c)
add_executable(sdl_pixel_graphics sdl_pixel_graphics.c)
target_include_directories(sdl_pixel_graphics PUBLIC inc)
# ToDo - windows, copy SDL2d.dll to location of binary...
# https://discourse.cmake.org/t/copying-created-shared-libraries/4936/4


if(EMCC_DETECTED)
    target_link_options(sdl_pixel_graphics PRIVATE -s USE_SDL=2 --shell-file "${CMAKE_CURRENT_SOURCE_DIR}/template.html" -sEXPORTED_RUNTIME_METHODS=[ccall] -sEXPORTED_FUNCTIONS=_jscallback,_main,_malloc,_free)

    target_compile_options(sdl_pixel_graphics PRIVATE -s USE_SDL=2)
    target_link_libraries(sdl_pixel_graphics nes_bus)

else()
    target_link_libraries(sdl_pixel_graphics SDL2main SDL2 nes_bus)
endif()
