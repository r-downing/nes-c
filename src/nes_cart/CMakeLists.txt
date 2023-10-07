
set(MAPPER_SOURCE_FILES
        mappers/mapper_000.c
        mappers/mapper_002.c
        mappers/mapper_003.c
        mappers/mapper_066.c
)

add_custom_command(
    OUTPUT mapper_table.c
    COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/mappers/make_mapper_table.py mapper_table.c "${MAPPER_SOURCE_FILES}"
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/mappers/make_mapper_table.py
)

add_library(nes_cart STATIC nes_cart.c ${CMAKE_CURRENT_BINARY_DIR}/mapper_table.c ${MAPPER_SOURCE_FILES})
target_include_directories(nes_cart PUBLIC inc)

add_executable(test_nes_cart tests/test_nes_cart.cpp)
target_link_libraries(test_nes_cart test_runner CppUTest CppUTestExt nes_cart)
add_test(NAME test_nes_cart COMMAND test_nes_cart)

add_executable(test_mapper tests/test_mapper_002.cpp)
target_link_libraries(test_mapper test_runner CppUTest CppUTestExt nes_cart)
add_test(NAME test_mapper COMMAND test_mapper)
