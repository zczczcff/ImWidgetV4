add_executable(TextListDemo
    main.cpp
)

target_link_libraries(TextListDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(TextListDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

if(WIN32)
    set_target_properties(TextListDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
