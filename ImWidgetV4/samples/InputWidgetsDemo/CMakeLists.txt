set(IMWIDGETV4_INPUT_WIDGETS_DEMO_SHARED_SOURCES
    DemoContent.cpp
)

if(TARGET imwidgetv4_platform_win32_dx11)
    add_executable(InputWidgetsDemo
        main.cpp
        ${IMWIDGETV4_INPUT_WIDGETS_DEMO_SHARED_SOURCES}
    )

    target_link_libraries(InputWidgetsDemo PRIVATE
        imwidgetv4_core
        imwidgetv4_platform_win32_dx11
    )

    set_target_properties(InputWidgetsDemo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/samples
    )

    if(WIN32)
        set_target_properties(InputWidgetsDemo PROPERTIES
            WIN32_EXECUTABLE TRUE
        )
    endif()
endif()

if(TARGET imwidgetv4_platform_android_gles3)
    add_library(InputWidgetsDemo SHARED
        AndroidMain.cpp
        ${IMWIDGETV4_INPUT_WIDGETS_DEMO_SHARED_SOURCES}
        ${IMWIDGETV4_ANDROID_NATIVE_APP_GLUE_SOURCE}
    )

    target_link_libraries(InputWidgetsDemo PRIVATE
        imwidgetv4_core
        imwidgetv4_platform_android_gles3
        android
        log
    )

    if(IMWIDGETV4_ANDROID_NDK_ROOT)
        target_include_directories(InputWidgetsDemo PRIVATE
            ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue
        )
    endif()

    set_target_properties(InputWidgetsDemo PROPERTIES
        FOLDER "Samples"
    )
endif()
