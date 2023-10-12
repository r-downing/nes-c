
set(MAPPER_SOURCE_FILES
        mappers/mapper_000.c
        mappers/mapper_002.c
        mappers/mapper_003.c
        mappers/mapper_004.c
        mappers/mapper_066.c
)

find_package(Python COMPONENTS Interpreter REQUIRED)
add_custom_command(
    OUTPUT mapper_table.c
    COMMAND ${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/mappers/make_mapper_table.py mapper_table.c ${MAPPER_SOURCE_FILES}
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mappers/make_mapper_table.py
)

add_library(nes_cart STATIC nes_cart.c ${CMAKE_CURRENT_BINARY_DIR}/mapper_table.c ${MAPPER_SOURCE_FILES})
target_include_directories(nes_cart PUBLIC inc)

add_executable(test_nes_cart tests/test_nes_cart.cpp)
target_link_libraries(test_nes_cart test_runner CppUTest CppUTestExt nes_cart)
add_test(NAME test_nes_cart COMMAND test_nes_cart)

add_executable(test_mapper_002 tests/test_mapper_002.cpp)
target_link_libraries(test_mapper_002 test_runner CppUTest CppUTestExt nes_cart)
add_test(NAME test_mapper_002 COMMAND test_mapper_002)

add_executable(test_mapper_004 tests/test_mapper_004.cpp)
target_link_libraries(test_mapper_004 test_runner CppUTest CppUTestExt nes_cart)
add_test(NAME test_mapper_004 COMMAND test_mapper_004)
