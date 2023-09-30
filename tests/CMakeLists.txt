
add_library(test_runner test_runner.cpp)

target_link_libraries(test_runner CppUTest)

if(EMCC_DETECTED)
    target_link_options(test_runner PRIVATE -sNO_DISABLE_EXCEPTION_CATCHING)
endif()


add_executable(test_nestest test_nestest.c)
target_link_libraries(test_nestest c6502 nes_cart nes_bus)

file(COPY assets/nestest.nes DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

find_package(Python COMPONENTS Interpreter REQUIRED)
add_test(NAME test_nestest COMMAND ${Python_EXECUTABLE} "${CMAKE_CURRENT_SOURCE_DIR}/nes_test_driver.py" ${CMAKE_CURRENT_BINARY_DIR}/test_nestest "${CMAKE_CURRENT_SOURCE_DIR}/assets/nestest.log")

add_subdirectory(nes_test_roms)
