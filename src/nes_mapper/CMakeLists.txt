
add_library(nes_mapper STATIC nes_mapper.c)
# target_link_libraries(nes_mapper c6502)
target_include_directories(nes_mapper PUBLIC inc)
