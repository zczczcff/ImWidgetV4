add_executable(CanvasPanelDemo
    main.cpp
)

target_link_libraries(CanvasPanelDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(CanvasPanelDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

if(WIN32)
    set_target_properties(CanvasPanelDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
