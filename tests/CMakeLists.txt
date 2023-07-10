
# based on https://github.com/davisford/cmake-cpputest

project (Tutorial)


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -DTEST_DEFINE -sNO_DISABLE_EXCEPTION_CATCHING")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -sNO_DISABLE_EXCEPTION_CATCHING")
set(CPPUTEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../lib/cpputest")

include_directories(${CPPUTEST_DIR}/include)

# add cpputest library
add_library(imp_cpputest STATIC IMPORTED)
set_property(TARGET imp_cpputest PROPERTY
             IMPORTED_LOCATION ${CPPUTEST_DIR}/src/CppUTest/libCppUTest.a)

add_executable(tests tests.cpp)
target_link_libraries(tests imp_cpputest)
