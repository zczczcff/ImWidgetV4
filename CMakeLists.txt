cmake_minimum_required(VERSION 3.15)
project(ImWidgetWorkspace VERSION 0.1.0 LANGUAGES CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(IMWIDGETV4_BUILD_TESTS "Build ImWidgetV4 tests" ON)
option(IMWIDGETV4_BUILD_SAMPLES "Build ImWidgetV4 samples" ON)
option(IMWIDGETV4_BUILD_EDITOR "Build ImWidgetV4 editor" ON)

add_subdirectory(ImWidgetV4)

if(IMWIDGETV4_BUILD_EDITOR)
    add_subdirectory(ImWidgetV4Editor)
endif()
