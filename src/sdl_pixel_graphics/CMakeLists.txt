
# add_library(sdl_pixel_graphics STATIC sdl_pixel_graphics.c)
add_executable(sdl_pixel_graphics sdl_pixel_graphics.c)
target_link_libraries(sdl_pixel_graphics SDL2main SDL2 nes_cart)
target_include_directories(sdl_pixel_graphics PUBLIC inc)
# ToDo - windows, copy SDL2d.dll to location of binary...
# https://discourse.cmake.org/t/copying-created-shared-libraries/4936/4


