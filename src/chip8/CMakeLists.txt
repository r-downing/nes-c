
add_library(chip8 STATIC chip8.c chip8_font_set.c chip8_impl.c)
target_include_directories(chip8 PUBLIC inc)
