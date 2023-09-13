
add_library(nes_gamepad STATIC nes_gamepad.c)
target_include_directories(nes_gamepad PUBLIC inc)

add_executable(test_nes_gamepad tests/test_nes_gamepad.cpp)
target_link_libraries(test_nes_gamepad test_runner CppUTest CppUTestExt nes_gamepad)
add_test(NAME test_nes_gamepad COMMAND test_nes_gamepad)
