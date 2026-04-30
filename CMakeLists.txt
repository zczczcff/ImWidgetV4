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
)
target_include_directories(imgui_lib PUBLIC ${imgui_SOURCE_DIR})

# 创建 stb 接口库
add_library(stb_lib INTERFACE)
target_include_directories(stb_lib INTERFACE ${stb_SOURCE_DIR})

# 创建 imwidgetv4_core 库目标
add_library(imwidgetv4_core STATIC
    ${CMAKE_SOURCE_DIR}/src/core/ReflectableObject.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Types.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Application.cpp
    ${CMAKE_SOURCE_DIR}/src/core/Widget.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/DrawContext.cpp
    ${CMAKE_SOURCE_DIR}/src/rendering/DrawUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/style/StyleSet.cpp
    ${CMAKE_SOURCE_DIR}/src/widgets/TextBlock.cpp
)
target_include_directories(imwidgetv4_core PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)
target_link_libraries(imwidgetv4_core PUBLIC
    nlohmann_json::nlohmann_json
    imgui_lib
    uectti
)

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
add_subdirectory(tests/rendering)
