
add_library(test_runner test_runner.cpp)

target_link_libraries(test_runner CppUTest)

if(EMCC_DETECTED)
    target_link_options(test_runner PRIVATE -sNO_DISABLE_EXCEPTION_CATCHING)
endif()
