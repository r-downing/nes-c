
add_library(nes_bus STATIC nes_bus.c)
target_link_libraries(nes_bus c6502 c2C02 nes_cart nes_gamepad)
target_include_directories(nes_bus PUBLIC inc)

add_executable(test_nes_bus tests/test_nes_bus.cpp)
target_link_libraries(test_nes_bus test_runner CppUTest CppUTestExt nes_bus)
add_test(NAME test_nes_bus COMMAND test_nes_bus)


