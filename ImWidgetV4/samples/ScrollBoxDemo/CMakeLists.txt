add_executable(ScrollBoxDemo
    main.cpp
)

target_link_libraries(ScrollBoxDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(ScrollBoxDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples
)

if(WIN32)
    set_target_properties(ScrollBoxDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
