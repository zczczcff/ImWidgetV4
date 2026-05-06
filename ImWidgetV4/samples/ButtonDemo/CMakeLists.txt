add_executable(ButtonDemo
    main.cpp
)

target_link_libraries(ButtonDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(ButtonDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples
)

if(WIN32)
    set_target_properties(ButtonDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
