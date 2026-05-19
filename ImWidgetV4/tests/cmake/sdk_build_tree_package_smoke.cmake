if(NOT IMWIDGETV4_PACKAGE_DIR)
    message(FATAL_ERROR "IMWIDGETV4_PACKAGE_DIR is required")
endif()

if(NOT IMWIDGETV4_MAIN_BINARY_DIR)
    message(FATAL_ERROR "IMWIDGETV4_MAIN_BINARY_DIR is required")
endif()

if(NOT IMWIDGETV4_SMOKE_BINARY_DIR)
    message(FATAL_ERROR "IMWIDGETV4_SMOKE_BINARY_DIR is required")
endif()

if(NOT IMWIDGETV4_SMOKE_CONFIG)
    set(IMWIDGETV4_SMOKE_CONFIG Debug)
endif()

set(smoke_source_dir "${IMWIDGETV4_SMOKE_BINARY_DIR}/src")
set(smoke_build_dir "${IMWIDGETV4_SMOKE_BINARY_DIR}/build")

file(REMOVE_RECURSE "${IMWIDGETV4_SMOKE_BINARY_DIR}")
file(MAKE_DIRECTORY "${smoke_source_dir}")

file(WRITE "${smoke_source_dir}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.24)
project(ImWidgetV4SdkPackageSmoke LANGUAGES CXX)

find_package(ImWidgetV4 CONFIG REQUIRED)

add_executable(package_smoke WIN32 main.cpp)
target_link_libraries(package_smoke PRIVATE
    ImWidgetV4::core
    ImWidgetV4::platform_win32_dx11
    ImWidgetV4::app_host_win32_main
)
]=])

file(WRITE "${smoke_source_dir}/main.cpp" [=[
#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/TextBlock.h>

#include <memory>

class SmokeHostDelegate final : public ImWidgetV4::IApplicationHostDelegate
{
public:
    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override
    {
        ImWidgetV4::FApplicationHostConfig config;
        config.Title = "SDK smoke";
        return config;
    }

    void ConfigureApplication(ImWidgetV4::ImApplication& application) override
    {
        auto widget = std::make_shared<ImWidgetV4::ImTextBlock>();
        widget->SetText("SDK smoke");
        application.SetRootWidget(widget);
    }
};

std::shared_ptr<ImWidgetV4::IApplicationHostDelegate> ImWidgetV4::CreateApplicationHostDelegate()
{
    return std::make_shared<SmokeHostDelegate>();
}
]=])

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        --build "${IMWIDGETV4_MAIN_BINARY_DIR}"
        --config "${IMWIDGETV4_SMOKE_CONFIG}"
        --target imwidgetv4_core imwidgetv4_platform_win32_dx11 imwidgetv4_app_host_win32_main
    RESULT_VARIABLE build_library_result
)
if(NOT build_library_result EQUAL 0)
    message(FATAL_ERROR "Failed to build ImWidgetV4 libraries for SDK package smoke test")
endif()

set(configure_command
    "${CMAKE_COMMAND}"
    -S "${smoke_source_dir}"
    -B "${smoke_build_dir}"
    "-DImWidgetV4_DIR=${IMWIDGETV4_PACKAGE_DIR}"
)

if(IMWIDGETV4_SMOKE_GENERATOR)
    list(APPEND configure_command -G "${IMWIDGETV4_SMOKE_GENERATOR}")
endif()
if(IMWIDGETV4_SMOKE_GENERATOR_PLATFORM)
    list(APPEND configure_command -A "${IMWIDGETV4_SMOKE_GENERATOR_PLATFORM}")
endif()
if(IMWIDGETV4_SMOKE_GENERATOR_TOOLSET)
    list(APPEND configure_command -T "${IMWIDGETV4_SMOKE_GENERATOR_TOOLSET}")
endif()

execute_process(
    COMMAND ${configure_command}
    RESULT_VARIABLE configure_result
)
if(NOT configure_result EQUAL 0)
    message(FATAL_ERROR "Failed to configure SDK package smoke project")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        --build "${smoke_build_dir}"
        --config "${IMWIDGETV4_SMOKE_CONFIG}"
        --target package_smoke
    RESULT_VARIABLE smoke_build_result
)
if(NOT smoke_build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build SDK package smoke project")
endif()
