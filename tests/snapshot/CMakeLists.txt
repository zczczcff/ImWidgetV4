add_executable(snapshot_test
    snapshot_test.cpp
)

target_link_libraries(snapshot_test PRIVATE
    imwidgetv4_core
    imgui_lib
    gtest_main
)

add_test(NAME SnapshotTest COMMAND snapshot_test)
