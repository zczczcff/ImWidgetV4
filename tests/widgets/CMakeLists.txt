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

add_executable(widget_reflection_test
    widget_reflection_test.cpp
)

target_link_libraries(widget_reflection_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME WidgetReflectionTest COMMAND widget_reflection_test)

add_executable(checkbox_test
    checkbox_test.cpp
)

target_link_libraries(checkbox_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME CheckBoxTest COMMAND checkbox_test)

add_executable(canvas_panel_test
    canvas_panel_test.cpp
)

target_link_libraries(canvas_panel_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME CanvasPanelTest COMMAND canvas_panel_test)

add_executable(editable_text_test
    editable_text_test.cpp
)

target_link_libraries(editable_text_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME EditableTextTest COMMAND editable_text_test)

add_executable(expandable_box_test
    expandable_box_test.cpp
)

target_link_libraries(expandable_box_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME ExpandableBoxTest COMMAND expandable_box_test)

add_executable(image_test
    image_test.cpp
)

target_link_libraries(image_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME ImageTest COMMAND image_test)

add_executable(combobox_test
    combobox_test.cpp
)

target_link_libraries(combobox_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME ComboBoxTest COMMAND combobox_test)

add_executable(popup_menu_test
    popup_menu_test.cpp
)

target_link_libraries(popup_menu_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME PopupMenuTest COMMAND popup_menu_test)

add_executable(slider_test
    slider_test.cpp
)

target_link_libraries(slider_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME SliderTest COMMAND slider_test)

add_executable(scroll_box_test
    scroll_box_test.cpp
)

target_link_libraries(scroll_box_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME ScrollBoxTest COMMAND scroll_box_test)

add_executable(splitter_test
    splitter_test.cpp
)

target_link_libraries(splitter_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME SplitterTest COMMAND splitter_test)

add_executable(text_list_test
    text_list_test.cpp
)

target_link_libraries(text_list_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME TextListTest COMMAND text_list_test)

add_executable(text_outline_view_test
    text_outline_view_test.cpp
)

target_link_libraries(text_outline_view_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME TextOutlineViewTest COMMAND text_outline_view_test)

add_executable(outline_view_test
    outline_view_test.cpp
)

target_link_libraries(outline_view_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME OutlineViewTest COMMAND outline_view_test)

add_executable(user_widget_test
    user_widget_test.cpp
)

target_link_libraries(user_widget_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME UserWidgetTest COMMAND user_widget_test)
