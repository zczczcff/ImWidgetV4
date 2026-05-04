# BoxPanelDemo - 演示 HorizontalBox 和 VerticalBox 的使用

add_executable(BoxPanelDemo
    main.cpp
)

target_link_libraries(BoxPanelDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

# 设置输出目录
set_target_properties(BoxPanelDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

# Windows 平台设置为 GUI 应用
if(WIN32)
    set_target_properties(BoxPanelDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
