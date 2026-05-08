add_executable(input_source_test
    input_source_test.cpp
)

target_link_libraries(input_source_test PRIVATE
    imwidgetv4_platform_imgui
    gtest
    gtest_main
)

add_test(NAME InputSourceTest COMMAND input_source_test)
