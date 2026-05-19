if(NOT IMWIDGETV4_MAIN_BINARY_DIR)
    message(FATAL_ERROR "IMWIDGETV4_MAIN_BINARY_DIR is required")
endif()

if(NOT IMWIDGETV4_SMOKE_BINARY_DIR)
    message(FATAL_ERROR "IMWIDGETV4_SMOKE_BINARY_DIR is required")
endif()

if(NOT IMWIDGETV4_SMOKE_CONFIG)
    set(IMWIDGETV4_SMOKE_CONFIG Debug)
endif()

set(sdk_prefix "${IMWIDGETV4_SMOKE_BINARY_DIR}/sdk")
set(smoke_source_dir "${IMWIDGETV4_SMOKE_BINARY_DIR}/consumer-src")
set(smoke_build_dir "${IMWIDGETV4_SMOKE_BINARY_DIR}/consumer-build")

file(REMOVE_RECURSE "${IMWIDGETV4_SMOKE_BINARY_DIR}")
file(MAKE_DIRECTORY "${smoke_source_dir}")

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        --build "${IMWIDGETV4_MAIN_BINARY_DIR}"
        --config "${IMWIDGETV4_SMOKE_CONFIG}"
        --target imwidgetv4_core imwidgetv4_platform_win32_dx11 imwidgetv4_app_host_win32_main
    RESULT_VARIABLE build_library_result
)
if(NOT build_library_result EQUAL 0)
    message(FATAL_ERROR "Failed to build ImWidgetV4 libraries for SDK install smoke test")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        --install "${IMWIDGETV4_MAIN_BINARY_DIR}"
        --config "${IMWIDGETV4_SMOKE_CONFIG}"
        --prefix "${sdk_prefix}"
    RESULT_VARIABLE install_result
)
if(NOT install_result EQUAL 0)
    message(FATAL_ERROR "Failed to install ImWidgetV4 SDK smoke package")
endif()

file(WRITE "${smoke_source_dir}/CMakeLists.txt" [=[
cmake_minimum_required(VERSION 3.24)
project(ImWidgetV4InstalledSdkPackageSmoke LANGUAGES CXX)

find_package(ImWidgetV4 CONFIG REQUIRED)

add_executable(installed_sdk_smoke WIN32 main.cpp)
target_link_libraries(installed_sdk_smoke PRIVATE
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

class InstalledSmokeHostDelegate final : public ImWidgetV4::IApplicationHostDelegate
{
public:
    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override
    {
        ImWidgetV4::FApplicationHostConfig config;
        config.Title = "Installed SDK smoke";
        return config;
    }

    void ConfigureApplication(ImWidgetV4::ImApplication& application) override
    {
        auto widget = std::make_shared<ImWidgetV4::ImTextBlock>();
        widget->SetText("Installed SDK smoke");
        application.SetRootWidget(widget);
    }
};

std::shared_ptr<ImWidgetV4::IApplicationHostDelegate> ImWidgetV4::CreateApplicationHostDelegate()
{
    return std::make_shared<InstalledSmokeHostDelegate>();
}
]=])

set(configure_command
    "${CMAKE_COMMAND}"
    -S "${smoke_source_dir}"
    -B "${smoke_build_dir}"
    "-DImWidgetV4_DIR=${sdk_prefix}/cmake"
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
    message(FATAL_ERROR "Failed to configure installed SDK smoke project")
endif()

execute_process(
    COMMAND
        "${CMAKE_COMMAND}"
        --build "${smoke_build_dir}"
        --config "${IMWIDGETV4_SMOKE_CONFIG}"
        --target installed_sdk_smoke
    RESULT_VARIABLE smoke_build_result
)
if(NOT smoke_build_result EQUAL 0)
    message(FATAL_ERROR "Failed to build installed SDK smoke project")
endif()
