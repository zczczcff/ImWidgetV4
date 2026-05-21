cmake_minimum_required(VERSION 3.15)
project(ImWidgetWorkspace VERSION 0.1.0 LANGUAGES C CXX)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(IMWIDGETV4_BUILD_TESTS "Build ImWidgetV4 tests" ON)
option(IMWIDGETV4_BUILD_SAMPLES "Build ImWidgetV4 samples" ON)
option(IMWIDGETV4_BUILD_EDITOR "Build ImWidgetV4 editor" ON)
option(IMWIDGETV4_BUILD_CLI "Build ImWidgetV4 command line tools" ON)
option(IMWIDGETV4_BUILD_SDK_BUILDER "Build ImWidget SDK Builder UI" ON)
option(IMWIDGETV4_ENABLE_CPACK "Enable CPack packaging targets" ON)
set(IMWIDGETV4_INSTALL_SDK_SUBDIR "" CACHE STRING "Install the ImWidgetV4 SDK under this subdirectory, for example sdk.")

if(IMWIDGETV4_BUILD_TESTS)
    enable_testing()
endif()

if(IMWIDGETV4_INSTALL_SDK_SUBDIR)
    set(IMWIDGETV4_INSTALL_ROOT_DIR "${IMWIDGETV4_INSTALL_SDK_SUBDIR}" CACHE STRING "Root install subdirectory for ImWidgetV4 library artifacts." FORCE)
endif()

add_subdirectory(ImWidgetV4)

if(IMWIDGETV4_BUILD_EDITOR OR IMWIDGETV4_BUILD_CLI)
    add_subdirectory(ImWidgetV4Editor)
endif()

if(IMWIDGETV4_BUILD_SDK_BUILDER)
    add_subdirectory(ImWidgetSDKBuilder)
endif()

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/templates")
    install(
        DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/templates/"
        DESTINATION templates
        COMPONENT Templates
    )
endif()

if(IMWIDGETV4_ENABLE_CPACK)
    include(cmake/ImWidgetV4CPack.cmake)
endif()
