
add_executable(test_rom_tests test_rom_tests.cpp)
target_link_libraries(test_rom_tests test_runner CppUTest CppUTestExt nes_bus)
# add_test(NAME test_nes_gamepad COMMAND test_nes_gamepad)
add_compile_definitions(ROMS_FOLDER="${CMAKE_CURRENT_SOURCE_DIR}")
