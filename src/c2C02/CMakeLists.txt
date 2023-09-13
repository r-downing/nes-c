
add_library(c2C02 STATIC c2C02.c)
target_include_directories(c2C02 PUBLIC inc)

add_executable(test_c2C02 tests/test_c2C02.cpp)
target_link_libraries(test_c2C02 test_runner CppUTest CppUTestExt c2C02)
add_test(NAME test_c2C02 COMMAND test_c2C02)
