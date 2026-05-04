add_executable(application_test
    application_test.cpp
)

target_link_libraries(application_test PRIVATE
    imwidgetv4_core
    gtest
    gtest_main
)

add_test(NAME ApplicationTest COMMAND application_test)
