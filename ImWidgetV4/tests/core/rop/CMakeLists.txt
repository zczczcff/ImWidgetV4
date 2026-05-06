set(TEST_SOURCES
    rop_test.cpp
)

add_executable(rop_tests ${TEST_SOURCES})
target_link_libraries(rop_tests PRIVATE uectti gtest_main)
target_include_directories(rop_tests PRIVATE ${PROJECT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(rop_tests PRIVATE /W4 /EHsc /utf-8)
else()
    target_compile_options(rop_tests PRIVATE -Wall -Wextra -pedantic)
endif()

add_test(NAME rop_tests COMMAND rop_tests)
