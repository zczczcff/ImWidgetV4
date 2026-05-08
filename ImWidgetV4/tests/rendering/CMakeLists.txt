# 创建 draw_context_test 可执行文件
add_executable(draw_context_test
    draw_context_test.cpp
)

# 链接库
target_link_libraries(draw_context_test
    imwidgetv4_core
    imwidgetv4_imgui
    gtest_main
)

# 添加测试
add_test(NAME DrawContextTest COMMAND draw_context_test)
