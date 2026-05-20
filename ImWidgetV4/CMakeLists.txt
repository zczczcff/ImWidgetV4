cmake_minimum_required(VERSION 3.15)
project(ImWidgetV4 VERSION 0.1.0 LANGUAGES CXX)

include(CMakePackageConfigHelpers)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT DEFINED IMWIDGETV4_BUILD_TESTS)
    option(IMWIDGETV4_BUILD_TESTS "Build ImWidgetV4 tests" ON)
endif()

if(NOT DEFINED IMWIDGETV4_BUILD_SAMPLES)
    option(IMWIDGETV4_BUILD_SAMPLES "Build ImWidgetV4 samples" ON)
endif()

if(NOT DEFINED IMWIDGETV4_BUILD_WIN32_DX11_BACKEND)
    option(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND "Build the Win32 + DirectX 11 backend" ${WIN32})
endif()

if(NOT DEFINED IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND)
    option(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND "Build the Android + OpenGL ES 3 backend" ${ANDROID})
endif()

if(MSVC)
    add_compile_options(/utf-8)
endif()

set(IMWIDGETV4_INSTALL_ROOT_DIR "" CACHE STRING "Root install subdirectory for ImWidgetV4 library artifacts.")
if(IMWIDGETV4_INSTALL_ROOT_DIR)
    set(IMWIDGETV4_INSTALL_INCLUDE_DIR "${IMWIDGETV4_INSTALL_ROOT_DIR}/include")
    set(IMWIDGETV4_INSTALL_LIBRARY_DIR "${IMWIDGETV4_INSTALL_ROOT_DIR}/lib/$<CONFIG>")
    set(IMWIDGETV4_INSTALL_RUNTIME_DIR "${IMWIDGETV4_INSTALL_ROOT_DIR}/bin/$<CONFIG>")
    set(IMWIDGETV4_INSTALL_CMAKE_DIR "${IMWIDGETV4_INSTALL_ROOT_DIR}/cmake")
else()
    set(IMWIDGETV4_INSTALL_INCLUDE_DIR include)
    set(IMWIDGETV4_INSTALL_LIBRARY_DIR "lib/$<CONFIG>")
    set(IMWIDGETV4_INSTALL_RUNTIME_DIR "bin/$<CONFIG>")
    set(IMWIDGETV4_INSTALL_CMAKE_DIR cmake)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

function(imwidgetv4_set_target_folder target folder)
    if(TARGET ${target})
        set_target_properties(${target} PROPERTIES FOLDER ${folder})
    endif()
endfunction()

add_library(uectti INTERFACE)
add_library(ImWidgetV4::uectti ALIAS uectti)
target_include_directories(uectti INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/core/reflection>
)
imwidgetv4_set_target_folder(uectti "Libraries")

include(FetchContent)

FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.90.4
)

FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG master
)

FetchContent_MakeAvailable(imgui json stb)

add_library(imwidgetv4_imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
add_library(ImWidgetV4::imgui ALIAS imwidgetv4_imgui)
target_include_directories(imwidgetv4_imgui PUBLIC
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
    $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui/backends>
)
imwidgetv4_set_target_folder(imwidgetv4_imgui "ThirdParty")

add_library(stb_lib INTERFACE)
add_library(ImWidgetV4::stb ALIAS stb_lib)
target_include_directories(stb_lib INTERFACE
    $<BUILD_INTERFACE:${stb_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/stb>
)
imwidgetv4_set_target_folder(stb_lib "ThirdParty")

set(IMWIDGETV4_REFLECTION_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/reflection/Reflectable.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/reflection/ReflectionBuilder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/reflection/ReflectionRegistry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/reflection/ReflectionTypes.h
)

set(IMWIDGETV4_REFLECTION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/reflection/ReflectionRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/reflection/ReflectionTypes.cpp
)

add_library(imwidgetv4_reflection STATIC
    ${IMWIDGETV4_REFLECTION_PUBLIC_HEADERS}
    ${IMWIDGETV4_REFLECTION_SOURCES}
)
add_library(ImWidgetV4::reflection ALIAS imwidgetv4_reflection)
target_compile_features(imwidgetv4_reflection PUBLIC cxx_std_17)
target_include_directories(imwidgetv4_reflection PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}>
)
if(MSVC)
    target_compile_options(imwidgetv4_reflection PUBLIC /utf-8)
endif()
imwidgetv4_set_target_folder(imwidgetv4_reflection "Libraries")

set(IMWIDGETV4_CORE_PUBLIC_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/app/ApplicationHost.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Application.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/ApplicationBackend.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/CoreIcon.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/DragDrop.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/DrawContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/FileDialog.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Localization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/NamedAction.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/PropertyType.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/PropertyType_temp.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/ReflectableObject.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Reply.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Slot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Types.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/WindowManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/Widget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/detail/const_string.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/detail/parser.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/flat_type_map.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/named_type.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_hash.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_id.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_id_constant.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_index.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/type_name.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/reflection/uectti/uectti.hpp
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/core/rop/RunTimeObjectProperty.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/input/Input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/platform/PlatformPaths.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/platform/PlatformProcess.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/rendering/DrawContext.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/rendering/DrawUtils.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/snapshot/Snapshot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/style/StyleResolvers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/style/StyleSet.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/BoxSlot.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/Button.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/ButtonStyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/CanvasPanel.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/CheckBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/ColorPicker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/ComboBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/DesignerSurface.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/EditableText.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/ExpandableBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/HorizontalBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/HorizontalSplitter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/Image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/ListView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/OutlineView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/PanelWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/PopupMenu.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/ScrollBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/Slider.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/Switch.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/TabView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/TitleBar.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/TextList.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/TextBlock.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/TextOutlineView.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/UserWidget.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/VerticalBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/widgets/VerticalSplitter.h
)

set(IMWIDGETV4_CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/ApplicationHost.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Application.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/CoreIconData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/DrawContext.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Localization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/ReflectableObject.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Slot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Types.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/WindowManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/Widget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input/Input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/PlatformPaths.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/PlatformProcess.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/DrawUtils.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snapshot/Snapshot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/snapshot/TextureRegistry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/style/StyleResolvers.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/style/StyleSet.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/BoxSlot.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/Button.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/ButtonStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/CanvasPanel.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/CheckBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/ColorPicker.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/ComboBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/DesignerSurface.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/EditableText.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/ExpandableBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/HorizontalBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/HorizontalSplitter.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/Image.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/ListView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/OutlineView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/PanelWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/PopupMenu.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/ScrollBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/Slider.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/Switch.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/TabView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/TitleBar.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/TextList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/TextBlock.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/TextOutlineView.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/UserWidget.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/VerticalBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/widgets/VerticalSplitter.cpp
)

set(IMWIDGETV4_CORE_VS_ONLY_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/CoreIconData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/CoreIconMask.inl
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering/DrawContext.cpp
)

set(IMWIDGETV4_APP_HOST_WIN32_MAIN_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/Win32HostedMain.cpp
)
set(IMWIDGETV4_APP_HOST_ANDROID_MAIN_SOURCE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/app/AndroidHostedMain.cpp
)

set_source_files_properties(${IMWIDGETV4_CORE_VS_ONLY_FILES} PROPERTIES HEADER_FILE_ONLY TRUE)

add_library(imwidgetv4_core STATIC
    ${IMWIDGETV4_CORE_PUBLIC_HEADERS}
    ${IMWIDGETV4_CORE_SOURCES}
    ${IMWIDGETV4_CORE_VS_ONLY_FILES}
)
add_library(ImWidgetV4::core ALIAS imwidgetv4_core)
target_compile_features(imwidgetv4_core PUBLIC cxx_std_17)
target_include_directories(imwidgetv4_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${json_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/json>
)
target_link_libraries(imwidgetv4_core PUBLIC
    imwidgetv4_reflection
    imwidgetv4_imgui
    uectti
    stb_lib
)
if(MSVC)
    target_compile_options(imwidgetv4_core PUBLIC /utf-8)
endif()
imwidgetv4_set_target_folder(imwidgetv4_core "Libraries")

source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${IMWIDGETV4_CORE_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${IMWIDGETV4_CORE_SOURCES} ${IMWIDGETV4_CORE_VS_ONLY_FILES})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/include PREFIX "Header Files" FILES ${IMWIDGETV4_REFLECTION_PUBLIC_HEADERS})
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${IMWIDGETV4_REFLECTION_SOURCES})

set(IMWIDGETV4_PLATFORM_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/platform/ImGuiInputSource.h
)

set(IMWIDGETV4_PLATFORM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/ImGuiInputSource.cpp
)

add_library(imwidgetv4_platform_imgui STATIC
    ${IMWIDGETV4_PLATFORM_HEADERS}
    ${IMWIDGETV4_PLATFORM_SOURCES}
)
add_library(ImWidgetV4::platform_imgui ALIAS imwidgetv4_platform_imgui)
target_include_directories(imwidgetv4_platform_imgui PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}>
)
target_link_libraries(imwidgetv4_platform_imgui PUBLIC
    imwidgetv4_core
    imwidgetv4_imgui
)
imwidgetv4_set_target_folder(imwidgetv4_platform_imgui "Platforms")

set(IMWIDGETV4_PLATFORM_WIN32_DX11_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/platform/Win32DX11Backend.h
)

set(IMWIDGETV4_PLATFORM_WIN32_DX11_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/Win32DX11Backend.cpp
)

set(IMWIDGETV4_PLATFORM_ANDROID_GLES3_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/include/imwidgetv4/platform/AndroidGLES3Backend.h
)

set(IMWIDGETV4_PLATFORM_ANDROID_GLES3_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/platform/AndroidGLES3Backend.cpp
)

if(IMWIDGETV4_BUILD_WIN32_DX11_BACKEND)
    add_library(imwidgetv4_imgui_backend_win32 STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    )
    add_library(ImWidgetV4::imgui_backend_win32 ALIAS imwidgetv4_imgui_backend_win32)
    target_include_directories(imwidgetv4_imgui_backend_win32 PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui/backends>
    )
    target_link_libraries(imwidgetv4_imgui_backend_win32 PUBLIC
        imwidgetv4_imgui
    )
    imwidgetv4_set_target_folder(imwidgetv4_imgui_backend_win32 "ThirdParty")

    add_library(imwidgetv4_imgui_renderer_dx11 STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.cpp
    )
    add_library(ImWidgetV4::imgui_renderer_dx11 ALIAS imwidgetv4_imgui_renderer_dx11)
    target_include_directories(imwidgetv4_imgui_renderer_dx11 PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui/backends>
    )
    target_link_libraries(imwidgetv4_imgui_renderer_dx11 PUBLIC
        imwidgetv4_imgui
        d3d11.lib
        dxgi.lib
        d3dcompiler.lib
    )
    imwidgetv4_set_target_folder(imwidgetv4_imgui_renderer_dx11 "ThirdParty")

    add_library(imwidgetv4_platform_win32_dx11 STATIC
        ${IMWIDGETV4_PLATFORM_WIN32_DX11_HEADERS}
        ${IMWIDGETV4_PLATFORM_WIN32_DX11_SOURCES}
    )
    add_library(ImWidgetV4::platform_win32_dx11 ALIAS imwidgetv4_platform_win32_dx11)
    target_include_directories(imwidgetv4_platform_win32_dx11 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}>
    )
    target_link_libraries(imwidgetv4_platform_win32_dx11 PUBLIC
        imwidgetv4_platform_imgui
        imwidgetv4_imgui_backend_win32
        imwidgetv4_imgui_renderer_dx11
        Ole32.lib
        Shell32.lib
    )
    imwidgetv4_set_target_folder(imwidgetv4_platform_win32_dx11 "Platforms")
endif()

if(IMWIDGETV4_BUILD_ANDROID_GLES3_BACKEND)
    set(IMWIDGETV4_ANDROID_NDK_ROOT "")
    if(DEFINED CMAKE_ANDROID_NDK)
        set(IMWIDGETV4_ANDROID_NDK_ROOT ${CMAKE_ANDROID_NDK})
    elseif(DEFINED ANDROID_NDK)
        set(IMWIDGETV4_ANDROID_NDK_ROOT ${ANDROID_NDK})
    endif()

    set(IMWIDGETV4_ANDROID_NATIVE_APP_GLUE_SOURCE "")
    if(IMWIDGETV4_ANDROID_NDK_ROOT)
        set(IMWIDGETV4_ANDROID_NATIVE_APP_GLUE_SOURCE
            ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue/android_native_app_glue.c
        )
    endif()

    add_library(imwidgetv4_imgui_backend_android STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_android.cpp
    )
    add_library(ImWidgetV4::imgui_backend_android ALIAS imwidgetv4_imgui_backend_android)
    target_include_directories(imwidgetv4_imgui_backend_android PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui/backends>
    )
    target_link_libraries(imwidgetv4_imgui_backend_android PUBLIC
        imwidgetv4_imgui
        android
        log
    )
    imwidgetv4_set_target_folder(imwidgetv4_imgui_backend_android "ThirdParty")

    add_library(imwidgetv4_imgui_renderer_opengl3 STATIC
        ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )
    add_library(ImWidgetV4::imgui_renderer_opengl3 ALIAS imwidgetv4_imgui_renderer_opengl3)
    target_include_directories(imwidgetv4_imgui_renderer_opengl3 PUBLIC
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}>
        $<BUILD_INTERFACE:${imgui_SOURCE_DIR}/backends>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui/backends>
    )
    target_compile_definitions(imwidgetv4_imgui_renderer_opengl3 PUBLIC IMGUI_IMPL_OPENGL_ES3=1)
    target_link_libraries(imwidgetv4_imgui_renderer_opengl3 PUBLIC
        imwidgetv4_imgui
        EGL
        GLESv3
    )
    imwidgetv4_set_target_folder(imwidgetv4_imgui_renderer_opengl3 "ThirdParty")

    add_library(imwidgetv4_platform_android_gles3 STATIC
        ${IMWIDGETV4_PLATFORM_ANDROID_GLES3_HEADERS}
        ${IMWIDGETV4_PLATFORM_ANDROID_GLES3_SOURCES}
    )
    add_library(ImWidgetV4::platform_android_gles3 ALIAS imwidgetv4_platform_android_gles3)
    target_include_directories(imwidgetv4_platform_android_gles3 PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${IMWIDGETV4_INSTALL_INCLUDE_DIR}>
    )
    if(IMWIDGETV4_ANDROID_NDK_ROOT)
        target_include_directories(imwidgetv4_platform_android_gles3 PRIVATE
            ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue
        )
    endif()
    target_link_libraries(imwidgetv4_platform_android_gles3 PUBLIC
        imwidgetv4_platform_imgui
        imwidgetv4_imgui_backend_android
        imwidgetv4_imgui_renderer_opengl3
        android
        log
        EGL
        GLESv3
    )
    imwidgetv4_set_target_folder(imwidgetv4_platform_android_gles3 "Platforms")
endif()

if(TARGET imwidgetv4_platform_win32_dx11)
    add_library(imwidgetv4_app_host_win32_main STATIC
        ${IMWIDGETV4_APP_HOST_WIN32_MAIN_SOURCE}
    )
    add_library(ImWidgetV4::app_host_win32_main ALIAS imwidgetv4_app_host_win32_main)
    target_link_libraries(imwidgetv4_app_host_win32_main PUBLIC
        imwidgetv4_core
        imwidgetv4_platform_win32_dx11
    )
    imwidgetv4_set_target_folder(imwidgetv4_app_host_win32_main "Application")
endif()

if(TARGET imwidgetv4_platform_android_gles3)
    add_library(imwidgetv4_app_host_android_main STATIC
        ${IMWIDGETV4_APP_HOST_ANDROID_MAIN_SOURCE}
    )
    add_library(ImWidgetV4::app_host_android_main ALIAS imwidgetv4_app_host_android_main)
    if(IMWIDGETV4_ANDROID_NDK_ROOT)
        target_include_directories(imwidgetv4_app_host_android_main PRIVATE
            ${IMWIDGETV4_ANDROID_NDK_ROOT}/sources/android/native_app_glue
        )
    endif()
    target_link_libraries(imwidgetv4_app_host_android_main PUBLIC
        imwidgetv4_core
        imwidgetv4_platform_android_gles3
    )
    imwidgetv4_set_target_folder(imwidgetv4_app_host_android_main "Application")
endif()

source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/include
    PREFIX "Header Files"
    FILES
        ${IMWIDGETV4_PLATFORM_HEADERS}
        ${IMWIDGETV4_PLATFORM_WIN32_DX11_HEADERS}
        ${IMWIDGETV4_PLATFORM_ANDROID_GLES3_HEADERS})
source_group(
    TREE ${CMAKE_CURRENT_SOURCE_DIR}/src
    PREFIX "Source Files"
    FILES
        ${IMWIDGETV4_PLATFORM_SOURCES}
        ${IMWIDGETV4_PLATFORM_WIN32_DX11_SOURCES}
        ${IMWIDGETV4_PLATFORM_ANDROID_GLES3_SOURCES})

if(IMWIDGETV4_BUILD_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    imwidgetv4_set_target_folder(gtest "ThirdParty/GoogleTest")
    imwidgetv4_set_target_folder(gtest_main "ThirdParty/GoogleTest")
    imwidgetv4_set_target_folder(gmock "ThirdParty/GoogleTest")
    imwidgetv4_set_target_folder(gmock_main "ThirdParty/GoogleTest")

    add_subdirectory(tests/core/reflection)
    add_subdirectory(tests/core/rop)
    add_subdirectory(tests/core)
    add_subdirectory(tests/core/application)
    add_subdirectory(tests/input)
    add_subdirectory(tests/rendering)
    add_subdirectory(tests/snapshot)
    add_subdirectory(tests/widgets)

    imwidgetv4_set_target_folder(reflectable_object_test "Tests/Core")
    imwidgetv4_set_target_folder(light_reflection_test "Tests/Core")
    imwidgetv4_set_target_folder(delegate_test "Tests/Core")
    imwidgetv4_set_target_folder(application_test "Tests/Core/Application")
    imwidgetv4_set_target_folder(uectti_tests "Tests/Core/Reflection")
    imwidgetv4_set_target_folder(rop_tests "Tests/Core/ROP")
    imwidgetv4_set_target_folder(input_source_test "Tests/Input")
    imwidgetv4_set_target_folder(draw_context_test "Tests/Rendering")
    imwidgetv4_set_target_folder(snapshot_test "Tests/Snapshot")
    imwidgetv4_set_target_folder(drag_drop_test "Tests/Core")
    imwidgetv4_set_target_folder(button_test "Tests/Widgets")
    imwidgetv4_set_target_folder(localization_test "Tests/Widgets")
    imwidgetv4_set_target_folder(widget_reflection_test "Tests/Widgets")
    imwidgetv4_set_target_folder(checkbox_test "Tests/Widgets")
    imwidgetv4_set_target_folder(color_picker_test "Tests/Widgets")
    imwidgetv4_set_target_folder(canvas_panel_test "Tests/Widgets")
    imwidgetv4_set_target_folder(editable_text_test "Tests/Widgets")
    imwidgetv4_set_target_folder(expandable_box_test "Tests/Widgets")
    imwidgetv4_set_target_folder(image_test "Tests/Widgets")
    imwidgetv4_set_target_folder(combobox_test "Tests/Widgets")
    imwidgetv4_set_target_folder(popup_menu_test "Tests/Widgets")
    imwidgetv4_set_target_folder(slider_test "Tests/Widgets")
    imwidgetv4_set_target_folder(switch_test "Tests/Widgets")
    imwidgetv4_set_target_folder(scroll_box_test "Tests/Widgets")
    imwidgetv4_set_target_folder(splitter_test "Tests/Widgets")
    imwidgetv4_set_target_folder(tab_view_test "Tests/Widgets")
    imwidgetv4_set_target_folder(text_list_test "Tests/Widgets")
    imwidgetv4_set_target_folder(list_view_test "Tests/Widgets")
    imwidgetv4_set_target_folder(text_outline_view_test "Tests/Widgets")
    imwidgetv4_set_target_folder(outline_view_test "Tests/Widgets")
    imwidgetv4_set_target_folder(user_widget_test "Tests/Widgets")

    if(TARGET imwidgetv4_platform_win32_dx11)
        add_test(
            NAME sdk_build_tree_package_smoke
            COMMAND
                ${CMAKE_COMMAND}
                -DIMWIDGETV4_PACKAGE_DIR=${CMAKE_CURRENT_BINARY_DIR}/cmake
                -DIMWIDGETV4_MAIN_BINARY_DIR=${CMAKE_BINARY_DIR}
                -DIMWIDGETV4_SMOKE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests/sdk-package-smoke
                -DIMWIDGETV4_SMOKE_GENERATOR=${CMAKE_GENERATOR}
                -DIMWIDGETV4_SMOKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
                -DIMWIDGETV4_SMOKE_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
                -DIMWIDGETV4_SMOKE_CONFIG=$<CONFIG>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/sdk_build_tree_package_smoke.cmake
        )
        add_test(
            NAME sdk_install_tree_package_smoke
            COMMAND
                ${CMAKE_COMMAND}
                -DIMWIDGETV4_MAIN_BINARY_DIR=${CMAKE_BINARY_DIR}
                -DIMWIDGETV4_SMOKE_BINARY_DIR=${CMAKE_CURRENT_BINARY_DIR}/tests/sdk-install-smoke
                -DIMWIDGETV4_SMOKE_GENERATOR=${CMAKE_GENERATOR}
                -DIMWIDGETV4_SMOKE_GENERATOR_PLATFORM=${CMAKE_GENERATOR_PLATFORM}
                -DIMWIDGETV4_SMOKE_GENERATOR_TOOLSET=${CMAKE_GENERATOR_TOOLSET}
                -DIMWIDGETV4_SMOKE_CONFIG=$<CONFIG>
                -P ${CMAKE_CURRENT_SOURCE_DIR}/tests/cmake/sdk_install_tree_package_smoke.cmake
        )
    endif()
endif()

if(IMWIDGETV4_BUILD_SAMPLES AND (TARGET imwidgetv4_platform_win32_dx11 OR TARGET imwidgetv4_platform_android_gles3))
    add_subdirectory(samples)

    imwidgetv4_set_target_folder(ButtonDemo "Samples")
    imwidgetv4_set_target_folder(BoxPanelDemo "Samples")
    imwidgetv4_set_target_folder(CanvasPanelDemo "Samples")
    imwidgetv4_set_target_folder(ExpandableBoxDemo "Samples")
    imwidgetv4_set_target_folder(DragDropDemo "Samples")
    imwidgetv4_set_target_folder(ImageDemo "Samples")
    imwidgetv4_set_target_folder(InputWidgetsDemo "Samples")
    imwidgetv4_set_target_folder(ListViewDemo "Samples")
    imwidgetv4_set_target_folder(ReflectionInspectorDemo "Samples")
    imwidgetv4_set_target_folder(ScrollBoxDemo "Samples")
    imwidgetv4_set_target_folder(SplitterDemo "Samples")
    imwidgetv4_set_target_folder(TabViewDemo "Samples")
    imwidgetv4_set_target_folder(TextListDemo "Samples")
    imwidgetv4_set_target_folder(OutlineViewDemo "Samples")
    imwidgetv4_set_target_folder(UserWidgetDemo "Samples")
    imwidgetv4_set_target_folder(WindowDemo "Samples")
    imwidgetv4_set_target_folder(AndroidDemo "Samples")
endif()

set_target_properties(uectti PROPERTIES EXPORT_NAME uectti)
set_target_properties(imwidgetv4_imgui PROPERTIES EXPORT_NAME imgui)
set_target_properties(stb_lib PROPERTIES EXPORT_NAME stb)
set_target_properties(imwidgetv4_reflection PROPERTIES EXPORT_NAME reflection)
set_target_properties(imwidgetv4_core PROPERTIES EXPORT_NAME core)
set_target_properties(imwidgetv4_platform_imgui PROPERTIES EXPORT_NAME platform_imgui)

set(IMWIDGETV4_EXPORT_TARGETS
    uectti
    imwidgetv4_imgui
    stb_lib
    imwidgetv4_reflection
    imwidgetv4_core
    imwidgetv4_platform_imgui
)

if(TARGET imwidgetv4_imgui_backend_win32)
    set_target_properties(imwidgetv4_imgui_backend_win32 PROPERTIES EXPORT_NAME imgui_backend_win32)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_imgui_backend_win32)
endif()
if(TARGET imwidgetv4_imgui_renderer_dx11)
    set_target_properties(imwidgetv4_imgui_renderer_dx11 PROPERTIES EXPORT_NAME imgui_renderer_dx11)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_imgui_renderer_dx11)
endif()
if(TARGET imwidgetv4_platform_win32_dx11)
    set_target_properties(imwidgetv4_platform_win32_dx11 PROPERTIES EXPORT_NAME platform_win32_dx11)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_platform_win32_dx11)
endif()
if(TARGET imwidgetv4_imgui_backend_android)
    set_target_properties(imwidgetv4_imgui_backend_android PROPERTIES EXPORT_NAME imgui_backend_android)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_imgui_backend_android)
endif()
if(TARGET imwidgetv4_imgui_renderer_opengl3)
    set_target_properties(imwidgetv4_imgui_renderer_opengl3 PROPERTIES EXPORT_NAME imgui_renderer_opengl3)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_imgui_renderer_opengl3)
endif()
if(TARGET imwidgetv4_platform_android_gles3)
    set_target_properties(imwidgetv4_platform_android_gles3 PROPERTIES EXPORT_NAME platform_android_gles3)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_platform_android_gles3)
endif()
if(TARGET imwidgetv4_app_host_win32_main)
    set_target_properties(imwidgetv4_app_host_win32_main PROPERTIES EXPORT_NAME app_host_win32_main)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_app_host_win32_main)
endif()
if(TARGET imwidgetv4_app_host_android_main)
    set_target_properties(imwidgetv4_app_host_android_main PROPERTIES EXPORT_NAME app_host_android_main)
    list(APPEND IMWIDGETV4_EXPORT_TARGETS imwidgetv4_app_host_android_main)
endif()

set(IMWIDGETV4_PACKAGE_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake")

file(MAKE_DIRECTORY "${IMWIDGETV4_PACKAGE_DIR}")
export(
    TARGETS ${IMWIDGETV4_EXPORT_TARGETS}
    NAMESPACE ImWidgetV4::
    FILE "${IMWIDGETV4_PACKAGE_DIR}/ImWidgetV4Targets.cmake"
)
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ImWidgetV4Config.cmake.in"
    "${IMWIDGETV4_PACKAGE_DIR}/ImWidgetV4Config.cmake"
    @ONLY
)
write_basic_package_version_file(
    "${IMWIDGETV4_PACKAGE_DIR}/ImWidgetV4ConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(
    TARGETS ${IMWIDGETV4_EXPORT_TARGETS}
    EXPORT ImWidgetV4Targets
    ARCHIVE DESTINATION "${IMWIDGETV4_INSTALL_LIBRARY_DIR}"
    LIBRARY DESTINATION "${IMWIDGETV4_INSTALL_LIBRARY_DIR}"
    RUNTIME DESTINATION "${IMWIDGETV4_INSTALL_RUNTIME_DIR}"
    INCLUDES DESTINATION "${IMWIDGETV4_INSTALL_INCLUDE_DIR}"
)
install(
    DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    DESTINATION "${IMWIDGETV4_INSTALL_INCLUDE_DIR}"
)
set(IMWIDGETV4_IMGUI_INSTALL_HEADERS
    "${imgui_SOURCE_DIR}/imconfig.h"
    "${imgui_SOURCE_DIR}/imgui.h"
    "${imgui_SOURCE_DIR}/imgui_internal.h"
    "${imgui_SOURCE_DIR}/imstb_rectpack.h"
    "${imgui_SOURCE_DIR}/imstb_textedit.h"
    "${imgui_SOURCE_DIR}/imstb_truetype.h"
)
set(IMWIDGETV4_IMGUI_BACKEND_INSTALL_HEADERS
    "${imgui_SOURCE_DIR}/backends/imgui_impl_android.h"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_dx11.h"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.h"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3_loader.h"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_win32.h"
)
file(GLOB IMWIDGETV4_STB_INSTALL_HEADERS CONFIGURE_DEPENDS "${stb_SOURCE_DIR}/*.h")
install(
    FILES ${IMWIDGETV4_IMGUI_INSTALL_HEADERS}
    DESTINATION "${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui"
)
install(
    FILES ${IMWIDGETV4_IMGUI_BACKEND_INSTALL_HEADERS}
    DESTINATION "${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/imgui/backends"
)
install(
    DIRECTORY "${json_SOURCE_DIR}/include/"
    DESTINATION "${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/json"
)
install(
    FILES ${IMWIDGETV4_STB_INSTALL_HEADERS}
    DESTINATION "${IMWIDGETV4_INSTALL_INCLUDE_DIR}/imwidgetv4/third_party/stb"
)
install(
    EXPORT ImWidgetV4Targets
    NAMESPACE ImWidgetV4::
    FILE ImWidgetV4Targets.cmake
    DESTINATION "${IMWIDGETV4_INSTALL_CMAKE_DIR}"
)
install(
    FILES
        "${IMWIDGETV4_PACKAGE_DIR}/ImWidgetV4Config.cmake"
        "${IMWIDGETV4_PACKAGE_DIR}/ImWidgetV4ConfigVersion.cmake"
    DESTINATION "${IMWIDGETV4_INSTALL_CMAKE_DIR}"
)
