# ReflectableObject 测试
add_executable(reflectable_object_test
    reflectable_object_test.cpp
)

target_link_libraries(reflectable_object_test
    imwidgetv4_core
    gtest_main
    uectti
)

# 注册测试
add_test(NAME reflectable_object_test COMMAND reflectable_object_test)
