cmake_minimum_required(VERSION 3.15)
project(ImWidgetV4 VERSION 0.1.0 LANGUAGES CXX)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置 MSVC 编译器使用 UTF-8 编码
if(MSVC)
    add_compile_options(/utf-8)
endif()

# 添加 include 目录
include_directories(${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection)
include_directories(${CMAKE_SOURCE_DIR}/include)

# 创建 UECTTI 头文件库目标
add_library(uectti INTERFACE)
target_include_directories(uectti INTERFACE
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection
)

# 使用 FetchContent 添加外部依赖
include(FetchContent)

# Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)

# nlohmann/json
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

# stb
FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui json stb)

# 创建 ImGui 库目标
add_library(imgui_lib STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
)
target_include_directories(imgui_lib PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)
# 链接 DirectX 11 和 Windows 库
target_link_libraries(imgui_lib PUBLIC
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
)

# 创建 stb 接口库
add_library(stb_lib INTERFACE)
target_include_directories(stb_lib INTERFACE ${stb_SOURCE_DIR})

# 创建 imwidgetv4_core 库目标
set(IMWIDGETV4_CORE_PUBLIC_HEADERS
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/Application.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/ApplicationBackend.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/DrawContext.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/PropertyType.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/PropertyType_temp.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/ReflectableObject.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/Reply.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/Slot.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/Types.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/Window.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/WindowManager.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/Widget.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/detail/const_string.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/detail/parser.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/flat_type_map.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/named_type.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_hash.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_id.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_id_constant.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_index.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_name.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/uectti.hpp
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/core/rop/RunTimeObjectProperty.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/input/Input.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/rendering/DrawContext.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/rendering/DrawUtils.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/snapshot/Snapshot.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/style/StyleSet.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/BoxSlot.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/Button.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/ButtonStyle.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/CanvasPanel.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/CheckBox.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/ComboBox.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/EditableText.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/HorizontalBox.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/HorizontalSplitter.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/PanelWidget.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/Slider.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/TextBlock.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/VerticalBox.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/widgets/VerticalSplitter.h
)

set(IMWIDGETV4_CORE_SOURCES
    ${CMAKE_SOURCE_DIR}/src/core/Application.cpp
    ${CMAKE_SOURCE_DIR}/src/core/DrawContext.cpp
    ${CMAKE_SOURCE_DIR}/src/core/ReflectableObject.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Slot.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Window.cpp
    ${CMAKE_SOURCE_DIR}/src/core/WindowManager.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Widget.cpp
    ${CMAKE_SOURCE_DIR}/src/input/Input.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/DrawUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/snapshot/Snapshot.cpp
    ${CMAKE_SOURCE_DIR}/src/style/StyleSet.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/BoxSlot.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/Button.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/CanvasPanel.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/CheckBox.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/ComboBox.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/EditableText.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/HorizontalBox.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/HorizontalSplitter.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/PanelWidget.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/Slider.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/TextBlock.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/VerticalBox.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/VerticalSplitter.cpp
)

set(IMWIDGETV4_CORE_VS_ONLY_FILES
    ${CMAKE_SOURCE_DIR}/src/rendering/DrawContext.cpp
)

set_source_files_properties(${IMWIDGETV4_CORE_VS_ONLY_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(imwidgetv4_core STATIC
    ${IMWIDGETV4_CORE_PUBLIC_HEADERS}
    ${IMWIDGETV4_CORE_SOURCES}
    ${IMWIDGETV4_CORE_VS_ONLY_FILES}
)
target_include_directories(imwidgetv4_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(imwidgetv4_core PUBLIC
    nlohmann_json::nlohmann_json
    imgui_lib
    uectti
    stb_lib
)

source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "Header Files" FILES ${IMWIDGETV4_CORE_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Source Files" FILES ${IMWIDGETV4_CORE_SOURCES} ${IMWIDGETV4_CORE_VS_ONLY_FILES})

set(IMWIDGETV4_PLATFORM_HEADERS
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/platform/ImGuiInputSource.h
    ${CMAKE_SOURCE_DIR}/include/imwidgetv4/platform/Win32DX11Backend.h
)

set(IMWIDGETV4_PLATFORM_SOURCES
    ${CMAKE_SOURCE_DIR}/src/platform/ImGuiInputSource.cpp
    ${CMAKE_SOURCE_DIR}/src/platform/Win32DX11Backend.cpp
)

add_library(imwidgetv4_platform_win32_dx11 STATIC
    ${IMWIDGETV4_PLATFORM_HEADERS}
    ${IMWIDGETV4_PLATFORM_SOURCES}
)
target_include_directories(imwidgetv4_platform_win32_dx11 PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(imwidgetv4_platform_win32_dx11 PUBLIC
    imwidgetv4_core
    imgui_lib
)

source_group(TREE ${CMAKE_SOURCE_DIR}/include PREFIX "Header Files" FILES ${IMWIDGETV4_PLATFORM_HEADERS})
source_group(TREE ${CMAKE_SOURCE_DIR}/src PREFIX "Source Files" FILES ${IMWIDGETV4_PLATFORM_SOURCES})

# 启用测试
enable_testing()

# 添加 Google Test
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

# 添加测试子目录
add_subdirectory(tests/core/reflection)
add_subdirectory(tests/core/rop)
add_subdirectory(tests/core)
add_subdirectory(tests/core/application)
add_subdirectory(tests/input)
add_subdirectory(tests/rendering)
add_subdirectory(tests/snapshot)
add_subdirectory(tests/widgets)

# 添加演示程序选项
option(IMWIDGETV4_BUILD_EXAMPLES "构建控件演示程序" ON)

# 添加演示程序子目录
if(IMWIDGETV4_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
