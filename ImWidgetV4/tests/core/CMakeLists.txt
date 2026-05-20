add_executable(reflectable_object_test
    reflectable_object_test.cpp
)

target_link_libraries(reflectable_object_test
    imwidgetv4_core
    imwidgetv4_reflection_json
    gtest_main
    uectti
)

add_test(NAME reflectable_object_test COMMAND reflectable_object_test)

add_executable(light_reflection_test
    light_reflection_test.cpp
)

target_link_libraries(light_reflection_test
    imwidgetv4_reflection
    gtest_main
)

add_test(NAME light_reflection_test COMMAND light_reflection_test)

add_executable(reflection_json_test
    reflection_json_test.cpp
)

target_link_libraries(reflection_json_test
    imwidgetv4_reflection_json
    gtest_main
)

add_test(NAME reflection_json_test COMMAND reflection_json_test)

add_executable(delegate_test
    delegate_test.cpp
)

target_link_libraries(delegate_test
    imwidgetv4_core
    gtest_main
)

add_test(NAME delegate_test COMMAND delegate_test)

add_executable(named_action_test
    named_action_test.cpp
)

target_link_libraries(named_action_test
    imwidgetv4_core
    gtest_main
)

add_test(NAME named_action_test COMMAND named_action_test)

add_executable(style_set_test
    style_set_test.cpp
)

target_link_libraries(style_set_test
    imwidgetv4_core
    imwidgetv4_reflection_json
    gtest_main
)

add_test(NAME style_set_test COMMAND style_set_test)
