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

add_executable(checkbox_test
    checkbox_test.cpp
)

target_link_libraries(checkbox_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME CheckBoxTest COMMAND checkbox_test)

add_executable(editable_text_test
    editable_text_test.cpp
)

target_link_libraries(editable_text_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME EditableTextTest COMMAND editable_text_test)

add_executable(slider_test
    slider_test.cpp
)

target_link_libraries(slider_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME SliderTest COMMAND slider_test)

add_executable(splitter_test
    splitter_test.cpp
)

target_link_libraries(splitter_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME SplitterTest COMMAND splitter_test)
