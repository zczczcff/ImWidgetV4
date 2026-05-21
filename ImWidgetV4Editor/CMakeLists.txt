project(ImWidgetV4Editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT "")
if(ANDROID)
    if(DEFINED CMAKE_ANDROID_NDK)
        set(IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT ${CMAKE_ANDROID_NDK})
    elseif(DEFINED ANDROID_NDK)
        set(IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT ${ANDROID_NDK})
    endif()
endif()

set(IMWIDGETV4_EDITOR_HOST_MAIN_SOURCE "")
set(IMWIDGETV4_EDITOR_PLATFORM_TARGET "")
set(IMWIDGETV4_EDITOR_ANDROID_NATIVE_APP_GLUE_SOURCE "")
if(TARGET imwidgetv4_platform_win32_dx11)
    set(IMWIDGETV4_EDITOR_HOST_MAIN_SOURCE ../ImWidgetV4/src/app/Win32HostedMain.cpp)
    set(IMWIDGETV4_EDITOR_PLATFORM_TARGET imwidgetv4_platform_win32_dx11)
elseif(TARGET imwidgetv4_platform_android_gles3)
    set(IMWIDGETV4_EDITOR_HOST_MAIN_SOURCE ../ImWidgetV4/src/app/AndroidHostedMain.cpp)
    set(IMWIDGETV4_EDITOR_PLATFORM_TARGET imwidgetv4_platform_android_gles3)
    if(IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT)
        set(IMWIDGETV4_EDITOR_ANDROID_NATIVE_APP_GLUE_SOURCE
            ${IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT}/sources/android/native_app_glue/android_native_app_glue.c
        )
    endif()
else()
    if(IMWIDGETV4_BUILD_EDITOR)
        message(FATAL_ERROR "ImWidgetV4Editor requires a platform backend target.")
    endif()
endif()

set(IMWIDGETV4_EDITOR_CORE_SOURCES
    src/build/BuildController.cpp
    src/codegen/WidgetTreeToCppGenerator.cpp
    src/editor/EditorDocument.cpp
    src/editor/LogicalWidgetTree.cpp
    src/project/EditorProject.cpp
    src/serialization/WidgetFactory.cpp
    src/serialization/WidgetSerializer.cpp
    src/templates/ProjectScaffolder.cpp
    src/toolchains/EnvironmentProbe.cpp
    src/toolchains/PlatformConfiguration.cpp
)

add_library(ImWidgetV4EditorCore STATIC ${IMWIDGETV4_EDITOR_CORE_SOURCES})
add_library(ImWidgetV4::EditorCore ALIAS ImWidgetV4EditorCore)
target_link_libraries(ImWidgetV4EditorCore PUBLIC
    imwidgetv4_core
    imwidgetv4_reflection_json
)
target_include_directories(ImWidgetV4EditorCore PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/../ImWidgetV4/include
)
set_target_properties(ImWidgetV4EditorCore PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/editor
    FOLDER "Editor"
)

set(IMWIDGETV4_EDITOR_UI_SOURCES
    src/commands/AddWidgetCommand.cpp
    src/commands/CommandStack.cpp
    src/commands/DocumentSnapshotCommand.cpp
    src/commands/MoveWidgetCommand.cpp
    src/commands/ReflectablePropertyCommand.cpp
    src/commands/RemoveWidgetCommand.cpp
    src/editor/EditorDesignerSurfaceHost.cpp
    src/editor/EditorLocalization.cpp
    src/editor/EditorPaths.cpp
    src/editor/EditorSession.cpp
    src/editor/EditorShellHost.cpp
    src/editor/EditorTheme.cpp
    src/editor/EditorWidgetTreeHost.cpp
    src/editor/EditorWorkspaceController.cpp
    src/editor/InputDialog.cpp
    src/editor/NewAppProjectDialog.cpp
    src/editor/ProjectSettingsDialog.cpp
    src/editor/SelectionModel.cpp
    src/editor/WidgetTypeIcon.cpp
    src/inspector/ReflectionDetailsView.cpp
    src/palette/WidgetPaletteDragDrop.cpp
    src/palette/WidgetPaletteView.cpp
    src/tree/DocumentTreeViewBinder.cpp
    src/tree/WidgetTreeDragDrop.h
)

if(IMWIDGETV4_BUILD_EDITOR)
    if(NOT IMWIDGETV4_EDITOR_PLATFORM_TARGET)
        message(FATAL_ERROR "ImWidgetV4Editor requires a platform backend target.")
    endif()

    add_library(ImWidgetV4EditorUi STATIC ${IMWIDGETV4_EDITOR_UI_SOURCES})
    add_library(ImWidgetV4::EditorUi ALIAS ImWidgetV4EditorUi)
    target_link_libraries(ImWidgetV4EditorUi PUBLIC
        ImWidgetV4EditorCore
        imwidgetv4_designer
        ${IMWIDGETV4_EDITOR_PLATFORM_TARGET}
    )
    target_include_directories(ImWidgetV4EditorUi PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../ImWidgetV4/include
    )
    set_target_properties(ImWidgetV4EditorUi PROPERTIES
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/editor
        FOLDER "Editor"
    )

    if(ANDROID)
        add_library(ImWidgetV4Editor SHARED
            ${IMWIDGETV4_EDITOR_HOST_MAIN_SOURCE}
            ${IMWIDGETV4_EDITOR_ANDROID_NATIVE_APP_GLUE_SOURCE}
            src/main.cpp
        )
    else()
        add_executable(ImWidgetV4Editor
            ${IMWIDGETV4_EDITOR_HOST_MAIN_SOURCE}
            src/main.cpp
        )
    endif()

    target_link_libraries(ImWidgetV4Editor PRIVATE
        ImWidgetV4EditorUi
    )
    target_include_directories(ImWidgetV4Editor PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../ImWidgetV4/include
    )

    if(ANDROID AND IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT)
        target_include_directories(ImWidgetV4Editor PRIVATE
            ${IMWIDGETV4_EDITOR_ANDROID_NDK_ROOT}/sources/android/native_app_glue
        )
    endif()

    set_target_properties(ImWidgetV4Editor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/editor
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/editor
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/editor
        FOLDER "Editor"
    )

    if(WIN32)
        set_target_properties(ImWidgetV4Editor PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()

    install(
        TARGETS ImWidgetV4Editor
        RUNTIME DESTINATION tools
        LIBRARY DESTINATION tools
        COMPONENT Tools
    )
endif()

if(IMWIDGETV4_BUILD_CLI AND NOT ANDROID)
    add_executable(ImWidgetV4Cli
        src/cli/main.cpp
    )
    target_link_libraries(ImWidgetV4Cli PRIVATE
        ImWidgetV4EditorCore
    )
    target_include_directories(ImWidgetV4Cli PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../ImWidgetV4/include
    )
    set_target_properties(ImWidgetV4Cli PROPERTIES
        OUTPUT_NAME imwidgetv4
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/tools
        FOLDER "Tools"
    )
    install(
        TARGETS ImWidgetV4Cli
        RUNTIME DESTINATION tools
        COMPONENT Tools
    )
endif()

if(IMWIDGETV4_BUILD_TESTS AND TARGET gtest_main AND IMWIDGETV4_BUILD_EDITOR)
    add_executable(editor_selection_test
        ${IMWIDGETV4_EDITOR_UI_SOURCES}
        tests/codegen_test.cpp
        tests/editor_snapshot_test.cpp
        tests/editor_selection_test.cpp
    )

    target_link_libraries(editor_selection_test PRIVATE
        ImWidgetV4EditorCore
        imwidgetv4_designer
        ${IMWIDGETV4_EDITOR_PLATFORM_TARGET}
        gtest
        gtest_main
    )

    target_include_directories(editor_selection_test PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/tests
    )

    add_test(NAME EditorSelectionTest COMMAND editor_selection_test)
endif()

if(MSVC)
    target_compile_options(ImWidgetV4EditorCore PRIVATE /FS)
    if(TARGET ImWidgetV4EditorUi)
        target_compile_options(ImWidgetV4EditorUi PRIVATE /FS)
    endif()
    if(TARGET ImWidgetV4Editor)
        target_compile_options(ImWidgetV4Editor PRIVATE /FS)
    endif()
    if(TARGET ImWidgetV4Cli)
        target_compile_options(ImWidgetV4Cli PRIVATE /FS)
    endif()
    if(TARGET editor_selection_test)
        target_compile_options(editor_selection_test PRIVATE /FS)
    endif()
endif()
