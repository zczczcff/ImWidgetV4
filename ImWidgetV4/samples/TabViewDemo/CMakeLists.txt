add_executable(TabViewDemo
    ${IMWIDGETV4_APP_HOST_WIN32_MAIN_SOURCE}
    main.cpp
)

target_link_libraries(TabViewDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(TabViewDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples
)

if(WIN32)
    set_target_properties(TabViewDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
