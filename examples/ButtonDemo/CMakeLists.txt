# ButtonDemo 可执行文件
add_executable(ButtonDemo
    main.cpp
)

target_link_libraries(ButtonDemo PRIVATE
    imwidgetv4_core
)

# 设置输出目录
set_target_properties(ButtonDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/examples
)

# Windows 平台设置为 GUI 应用
if(WIN32)
    set_target_properties(ButtonDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
