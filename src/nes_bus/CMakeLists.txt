
add_library(nes_bus STATIC nes_bus.c)
target_link_libraries(nes_bus c6502 nes_cart)
target_include_directories(nes_bus PUBLIC inc)
