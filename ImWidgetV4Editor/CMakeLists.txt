project(ImWidgetV4Editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(ImWidgetV4Editor
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
    src/editor/EditorShellHost.cpp
    src/editor/InputDialog.cpp
    src/editor/NewAppProjectDialog.cpp
    src/editor/EditorSession.cpp
    src/editor/EditorWorkspaceController.cpp
    src/editor/LogicalWidgetTree.cpp
    src/editor/SelectionModel.cpp
    src/editor/WidgetTypeIcon.cpp
    src/inspector/ReflectionDetailsView.cpp
    src/project/EditorProject.cpp
    src/templates/ProjectScaffolder.cpp
    src/main.cpp
    src/palette/WidgetPaletteDragDrop.cpp
    src/palette/WidgetPaletteView.cpp
    src/serialization/WidgetFactory.cpp
    src/serialization/WidgetSerializer.cpp
    src/tree/DocumentTreeViewBinder.cpp
    src/tree/WidgetTreeDragDrop.h
)

if(IMWIDGETV4_BUILD_TESTS AND TARGET gtest_main)
    add_executable(editor_selection_test
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
        src/editor/EditorShellHost.cpp
        src/editor/InputDialog.cpp
        src/editor/NewAppProjectDialog.cpp
        src/editor/EditorSession.cpp
        src/editor/EditorWorkspaceController.cpp
        src/editor/LogicalWidgetTree.cpp
        src/editor/SelectionModel.cpp
        src/editor/WidgetTypeIcon.cpp
        src/inspector/ReflectionDetailsView.cpp
        src/project/EditorProject.cpp
        src/templates/ProjectScaffolder.cpp
        src/palette/WidgetPaletteDragDrop.cpp
        src/palette/WidgetPaletteView.cpp
        src/serialization/WidgetFactory.cpp
        src/serialization/WidgetSerializer.cpp
        src/tree/DocumentTreeViewBinder.cpp
        tests/codegen_test.cpp
        tests/editor_selection_test.cpp
    )

    target_link_libraries(editor_selection_test PRIVATE
        imwidgetv4_core
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
    imwidgetv4_platform_win32_dx11
)

target_include_directories(ImWidgetV4Editor PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

set_target_properties(ImWidgetV4Editor PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/editor
    WIN32_EXECUTABLE TRUE
    FOLDER "Editor"
)

if(MSVC)
    target_compile_options(ImWidgetV4Editor PRIVATE /FS)
    if(TARGET editor_selection_test)
        target_compile_options(editor_selection_test PRIVATE /FS)
    endif()
endif()
