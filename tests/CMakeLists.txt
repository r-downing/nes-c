
# based on https://github.com/davisford/cmake-cpputest

project (Tutorial)


set(CPPUTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/cpputest")

add_link_options(-sNO_DISABLE_EXCEPTION_CATCHING)


include_directories(${CPPUTEST_DIR}/include)

# add cpputest library
add_library(CppUTest STATIC IMPORTED)
set_property(TARGET CppUTest PROPERTY
             IMPORTED_LOCATION ${CPPUTEST_DIR}/cpputest_build/src/CppUTest/libCppUTest.a)

add_library(CppUTestExt STATIC IMPORTED)
set_property(TARGET CppUTestExt PROPERTY
            IMPORTED_LOCATION ${CPPUTEST_DIR}/cpputest_build/src/CppUTestExt/libCppUTestExt.a)

set(CMAKE_EXECUTABLE_SUFFIX ".js")
add_executable(tests tests.cpp)
target_link_libraries(tests CppUTest CppUTestExt chip8)
