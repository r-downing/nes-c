
# based on https://github.com/davisford/cmake-cpputest

project (Tutorial)


set(CPPUTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/cpputest")

add_link_options(-sNO_DISABLE_EXCEPTION_CATCHING)

set(CMAKE_EXECUTABLE_SUFFIX ".js")

include_directories(${CPPUTEST_DIR}/include)

# add cpputest library
add_library(imp_cpputest STATIC IMPORTED)
set_property(TARGET imp_cpputest PROPERTY
             IMPORTED_LOCATION ${CPPUTEST_DIR}/cpputest_build/src/CppUTest/libCppUTest.a)

add_executable(tests tests.cpp)
target_link_libraries(tests imp_cpputest)
