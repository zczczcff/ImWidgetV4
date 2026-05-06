add_executable(ExpandableBoxDemo
    main.cpp
)

target_link_libraries(ExpandableBoxDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(ExpandableBoxDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples
)

if(WIN32)
    set_target_properties(ExpandableBoxDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
