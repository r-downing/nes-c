
add_library(c6502 STATIC c6502.c)
target_include_directories(c6502 PUBLIC inc)

add_executable(test_c6502 test_c6502.cpp)
target_link_libraries(test_c6502 test_runner CppUTest CppUTestExt c6502)
add_test(NAME test_c6502 COMMAND test_c6502)
