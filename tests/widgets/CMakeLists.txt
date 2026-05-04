# Button 测试
add_executable(button_test
    button_test.cpp
)

target_link_libraries(button_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

# 添加测试
add_test(NAME ButtonTest COMMAND button_test)

add_executable(splitter_test
    splitter_test.cpp
)

target_link_libraries(splitter_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME SplitterTest COMMAND splitter_test)
