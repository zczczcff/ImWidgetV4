cmake_minimum_required(VERSION 3.24)
project(ImWidgetV4MinimalDesktopApp LANGUAGES CXX)

find_package(ImWidgetV4 CONFIG REQUIRED)

add_executable(ImWidgetV4MinimalDesktopApp WIN32
    main.cpp
)

target_compile_features(ImWidgetV4MinimalDesktopApp PRIVATE cxx_std_17)

target_link_libraries(ImWidgetV4MinimalDesktopApp PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_win32_dx11
    ImWidgetV4::app_host_win32_main
)
