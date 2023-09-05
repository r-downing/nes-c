
add_library(c6502 STATIC c6502.c)
target_include_directories(c6502 PUBLIC inc)

add_executable(test_c6502 tests/test_c6502.cpp)
target_link_libraries(test_c6502 test_runner CppUTest CppUTestExt c6502)
add_test(NAME test_c6502 COMMAND test_c6502)


add_executable(nes_test_runner tests/nes_test_runner.c)
target_link_libraries(nes_test_runner c6502)

file(COPY tests/nestest.nes DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

find_package(Python COMPONENTS Interpreter REQUIRED)
add_test(NAME nes_test COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/tests/nes_test_driver.py" ${CMAKE_CURRENT_BINARY_DIR}/nes_test_runner "${CMAKE_CURRENT_SOURCE_DIR}/tests/nestest.nes"  "${CMAKE_CURRENT_SOURCE_DIR}/tests/nestest.log")
