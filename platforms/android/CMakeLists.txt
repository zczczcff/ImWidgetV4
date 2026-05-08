cmake_minimum_required(VERSION 3.22.1)
project(ImWidgetAndroidShell LANGUAGES C CXX)

set(IMWIDGETV4_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(IMWIDGETV4_BUILD_SAMPLES ON CACHE BOOL "" FORCE)
set(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND OFF CACHE BOOL "" FORCE)
set(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND ON CACHE BOOL "" FORCE)

add_subdirectory(
    ${CMAKE_CURRENT_LIST_DIR}/../../ImWidgetV4
    ${CMAKE_CURRENT_BINARY_DIR}/imwidgetv4
)

if(NOT TARGET AndroidDemo)
    message(FATAL_ERROR "AndroidDemo target is required for the Android shell build.")
endif()
