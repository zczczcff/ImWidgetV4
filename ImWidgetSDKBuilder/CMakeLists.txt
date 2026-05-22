cmake_minimum_required(VERSION 3.15)

set(IMWIDGET_SDK_BUILDER_SOURCES
    src/main.cpp
    generated/AppProjectConfig.cpp
    generated/MainView.cpp
    generated/TitleBarView.cpp
)

if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/generated/AppResources.rc")
    list(APPEND IMWIDGET_SDK_BUILDER_SOURCES generated/AppResources.rc)
endif()

add_executable(ImWidgetSDKBuilder
    ${IMWIDGET_SDK_BUILDER_SOURCES}
)

target_include_directories(ImWidgetSDKBuilder PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/generated
)

target_link_libraries(ImWidgetSDKBuilder PRIVATE
    imwidgetv4_core
    imwidgetv4_reflection_json
)

if(WIN32)
    target_link_libraries(ImWidgetSDKBuilder PRIVATE
        imwidgetv4_platform_win32_dx11
        imwidgetv4_app_host_win32_main
    )
    set_target_properties(ImWidgetSDKBuilder PROPERTIES WIN32_EXECUTABLE TRUE)
else()
    message(FATAL_ERROR "ImWidgetSDKBuilder currently supports only the Windows desktop host.")
endif()

if(MSVC)
    target_compile_options(ImWidgetSDKBuilder PRIVATE /utf-8)
endif()

set_target_properties(ImWidgetSDKBuilder PROPERTIES FOLDER "Tools")
