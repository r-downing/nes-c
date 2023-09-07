
add_library(nes_cart STATIC nes_cart.c mappers/mapper0.c)
target_include_directories(nes_cart PUBLIC inc)
