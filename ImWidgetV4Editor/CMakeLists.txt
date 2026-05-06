project(ImWidgetV4Editor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(ImWidgetV4Editor
    src/editor/EditorDocument.cpp
    src/editor/EditorSession.cpp
    src/main.cpp
    src/serialization/WidgetFactory.cpp
    src/serialization/WidgetSerializer.cpp
)

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
