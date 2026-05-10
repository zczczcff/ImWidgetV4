cmake_minimum_required(VERSION 3.15)
project(ImWidgetWorkspace VERSION 0.1.0 LANGUAGES C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(IMWIDGETV4_BUILD_TESTS "Build ImWidgetV4 tests" ON)
option(IMWIDGETV4_BUILD_SAMPLES "Build ImWidgetV4 samples" ON)
option(IMWIDGETV4_BUILD_EDITOR "Build ImWidgetV4 editor" ON)

if(IMWIDGETV4_BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(ImWidgetV4)

if(IMWIDGETV4_BUILD_EDITOR AND (TARGET imwidgetv4_platform_win32_dx11 OR TARGET imwidgetv4_platform_android_gles3))
    add_subdirectory(ImWidgetV4Editor)
endif()
