set(IMWIDGETV4_ANDROID_NDK_ROOT "")
if(DEFINED CMAKE_ANDROID_NDK)
    set(IMWIDGETV4_ANDROID_NDK_ROOT ${CMAKE_ANDROID_NDK})
elseif(DEFINED ANDROID_NDK)
    set(IMWIDGETV4_ANDROID_NDK_ROOT ${ANDROID_NDK})
endif()

set(ANDROID_NATIVE_APP_GLUE_SOURCE "")
if(IMWIDGETV4_ANDROID_NDK_ROOT)
    set(ANDROID_NATIVE_APP_GLUE_SOURCE
        ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue/android_native_app_glue.c
    )
endif()

add_library(AndroidDemo SHARED
    ${IMWIDGETV4_APP_HOST_ANDROID_MAIN_SOURCE}
    main.cpp
    ${ANDROID_NATIVE_APP_GLUE_SOURCE}
)

target_link_libraries(AndroidDemo PRIVATE
    imwidgetv4_core
    imwidgetv4_platform_android_gles3
    android
    log
)

if(IMWIDGETV4_ANDROID_NDK_ROOT)
    target_include_directories(AndroidDemo PRIVATE
        ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue
    )
endif()

set_target_properties(AndroidDemo PROPERTIES
    FOLDER "Samples"
)
