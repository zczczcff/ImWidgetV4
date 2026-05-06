add_executable(input_source_test
    input_source_test.cpp
)

target_link_libraries(input_source_test PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
    gtest
    gtest_main
)

add_test(NAME InputSourceTest COMMAND input_source_test)
