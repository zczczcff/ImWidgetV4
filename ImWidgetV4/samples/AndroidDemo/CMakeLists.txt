set(ANDROID_NATIVE_APP_GLUE_SOURCE "")
if(DEFINED ANDROID_NDK)
    set(ANDROID_NATIVE_APP_GLUE_SOURCE
        ${ANDROID_NDK}/sources/android/native_app_glue/android_native_app_glue.c
    )
endif()

add_library(AndroidDemo SHARED
    main.cpp
    ${ANDROID_NATIVE_APP_GLUE_SOURCE}
)

target_link_libraries(AndroidDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_android_gles3
    android
    log
)

if(DEFINED ANDROID_NDK)
    target_include_directories(AndroidDemo PRIVATE
        ${ANDROID_NDK}/sources/android/native_app_glue
    )
endif()

set_target_properties(AndroidDemo PROPERTIES
    FOLDER "Samples"
)
