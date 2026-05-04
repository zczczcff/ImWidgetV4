#pragma once

#include <filesystem>
#include <system_error>

namespace ImWidgetV4::Examples {

inline std::filesystem::path GetDefaultDemoImGuiIniPath(const wchar_t* fileName)
{
    const std::filesystem::path directory = std::filesystem::path(L"C:\\ImWidgetV4\\imgui");
    std::error_code errorCode;
    std::filesystem::create_directories(directory, errorCode);
    return directory / fileName;
}

} // namespace ImWidgetV4::Examples
