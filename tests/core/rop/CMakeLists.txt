include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

set(TEST_SOURCES
    rop_test.cpp
)

add_executable(rop_tests ${TEST_SOURCES})
target_link_libraries(rop_tests PRIVATE uectti gtest_main)
target_include_directories(rop_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)

# 设置编译选项
if(MSVC)
    target_compile_options(rop_tests PRIVATE /W4 /EHsc /utf-8)
else()
    target_compile_options(rop_tests PRIVATE -Wall -Wextra -pedantic)
endif()

add_test(NAME rop_tests COMMAND rop_tests)
