
add_library(nes_cart STATIC nes_cart.c mappers/mapper0.c)
target_include_directories(nes_cart PUBLIC inc)

add_executable(test_nes_cart tests/test_nes_cart.cpp)
target_link_libraries(test_nes_cart test_runner CppUTest CppUTestExt nes_cart)
add_test(NAME test_nes_cart COMMAND test_nes_cart)
