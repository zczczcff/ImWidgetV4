add_executable(reflectable_object_test
    reflectable_object_test.cpp
)

target_link_libraries(reflectable_object_test
    imwidgetv4_core
    gtest_main
    uectti
)

add_test(NAME reflectable_object_test COMMAND reflectable_object_test)

add_executable(delegate_test
    delegate_test.cpp
)

target_link_libraries(delegate_test
    imwidgetv4_core
    gtest_main
)

add_test(NAME delegate_test COMMAND delegate_test)
