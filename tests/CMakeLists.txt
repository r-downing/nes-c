
add_executable(tests test_runner.cpp test_chip8.cpp test_c6502.cpp)
target_link_libraries(tests CppUTest CppUTestExt chip8 c6502)

if(EMCC_DETECTED)
    target_link_options(tests PRIVATE -sNO_DISABLE_EXCEPTION_CATCHING)
endif()
