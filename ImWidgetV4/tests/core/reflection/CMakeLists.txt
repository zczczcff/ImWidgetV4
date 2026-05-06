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
    const_string_test.cpp
    type_id_test.cpp
    type_name_test.cpp
    type_hash_test.cpp
    type_index_test.cpp
    type_id_constant_test.cpp
    named_type_test.cpp
    flat_type_map_test.cpp
)

add_executable(uectti_tests ${TEST_SOURCES})
target_link_libraries(uectti_tests PRIVATE uectti gtest_main)

# 设置编译选项
if(MSVC)
    target_compile_options(uectti_tests PRIVATE /W4 /EHsc)
else()
    target_compile_options(uectti_tests PRIVATE -Wall -Wextra -pedantic)
endif()

add_test(NAME uectti_tests COMMAND uectti_tests)
