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
    message(FATAL_ERROR "ImWidgetV4Editor requires a platform backend target.")
endif()

set(IMWIDGETV4_EDITOR_SOURCES
    ${IMWIDGETV4_EDITOR_HOST_MAIN_SOURCE}
    ${IMWIDGETV4_EDITOR_ANDROID_NATIVE_APP_GLUE_SOURCE}
    src/build/BuildController.cpp
    src/codegen/WidgetTreeToCppGenerator.cpp
    src/commands/AddWidgetCommand.cpp
    src/commands/CommandStack.cpp
    src/commands/DocumentSnapshotCommand.cpp
    src/commands/MoveWidgetCommand.cpp
    src/commands/RemoveWidgetCommand.cpp
    src/commands/ReflectablePropertyCommand.cpp
    src/editor/EditorDocument.cpp
    src/editor/EditorPaths.cpp
    src/editor/EditorLocalization.cpp
    src/editor/EditorDesignerSurfaceHost.cpp
    src/editor/EditorShellHost.cpp
    src/editor/EditorTheme.cpp
    src/editor/EditorWidgetTreeHost.cpp
    src/editor/InputDialog.cpp
    src/editor/NewAppProjectDialog.cpp
    src/editor/ProjectSettingsDialog.cpp
    src/editor/EditorSession.cpp
    src/editor/EditorWorkspaceController.cpp
    src/editor/LogicalWidgetTree.cpp
    src/editor/SelectionModel.cpp
    src/editor/WidgetTypeIcon.cpp
    src/inspector/ReflectionDetailsView.cpp
    src/project/EditorProject.cpp
    src/templates/ProjectScaffolder.cpp
    src/toolchains/EnvironmentProbe.cpp
    src/toolchains/PlatformConfiguration.cpp
    src/main.cpp
    src/palette/WidgetPaletteDragDrop.cpp
    src/palette/WidgetPaletteView.cpp
    src/serialization/WidgetFactory.cpp
    src/serialization/WidgetSerializer.cpp
    src/tree/DocumentTreeViewBinder.cpp
    src/tree/WidgetTreeDragDrop.h
)

if(ANDROID)
    add_library(ImWidgetV4Editor SHARED ${IMWIDGETV4_EDITOR_SOURCES})
else()
    add_executable(ImWidgetV4Editor ${IMWIDGETV4_EDITOR_SOURCES})
endif()

if(IMWIDGETV4_BUILD_TESTS AND TARGET gtest_main)
    add_executable(editor_selection_test
        src/main.cpp
        src/build/BuildController.cpp
        src/codegen/WidgetTreeToCppGenerator.cpp
        src/commands/AddWidgetCommand.cpp
        src/commands/CommandStack.cpp
        src/commands/DocumentSnapshotCommand.cpp
        src/commands/MoveWidgetCommand.cpp
        src/commands/RemoveWidgetCommand.cpp
        src/commands/ReflectablePropertyCommand.cpp
        src/editor/EditorDocument.cpp
        src/editor/EditorPaths.cpp
        src/editor/EditorLocalization.cpp
        src/editor/EditorDesignerSurfaceHost.cpp
        src/editor/EditorShellHost.cpp
        src/editor/EditorTheme.cpp
        src/editor/EditorWidgetTreeHost.cpp
        src/editor/InputDialog.cpp
        src/editor/NewAppProjectDialog.cpp
        src/editor/ProjectSettingsDialog.cpp
        src/editor/EditorSession.cpp
        src/editor/EditorWorkspaceController.cpp
        src/editor/LogicalWidgetTree.cpp
        src/editor/SelectionModel.cpp
        src/editor/WidgetTypeIcon.cpp
        src/inspector/ReflectionDetailsView.cpp
        src/project/EditorProject.cpp
        src/templates/ProjectScaffolder.cpp
        src/toolchains/EnvironmentProbe.cpp
        src/toolchains/PlatformConfiguration.cpp
        src/palette/WidgetPaletteDragDrop.cpp
        src/palette/WidgetPaletteView.cpp
        src/serialization/WidgetFactory.cpp
        src/serialization/WidgetSerializer.cpp
        src/tree/DocumentTreeViewBinder.cpp
        tests/codegen_test.cpp
        tests/editor_snapshot_test.cpp
        tests/editor_selection_test.cpp
    )

    target_link_libraries(editor_selection_test PRIVATE
        imwidgetv4_core
        imwidgetv4_designer
        imwidgetv4_reflection_json
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

target_link_libraries(ImWidgetV4Editor PRIVATE
    imwidgetv4_core
    imwidgetv4_designer
    imwidgetv4_reflection_json
    ${IMWIDGETV4_EDITOR_PLATFORM_TARGET}
)

target_include_directories(ImWidgetV4Editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
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

if(MSVC)
    target_compile_options(ImWidgetV4Editor PRIVATE /FS)
    if(TARGET editor_selection_test)
        target_compile_options(editor_selection_test PRIVATE /FS)
    endif()
endif()

install(
    TARGETS ImWidgetV4Editor
    RUNTIME DESTINATION tools
    LIBRARY DESTINATION tools
    COMPONENT Tools
)
