add_executable(ReflectionInspectorDemo
    ${IMWIDGETV4_APP_HOST_WIN32_MAIN_SOURCE}
    main.cpp
)

target_link_libraries(ReflectionInspectorDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_win32_dx11
)

set_target_properties(ReflectionInspectorDemo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples
)

if(WIN32)
    set_target_properties(ReflectionInspectorDemo PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()
