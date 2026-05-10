#pragma once

#include <imwidgetv4/platform/PlatformPaths.h>
#include <filesystem>
#include <system_error>

namespace ImWidgetV4::Samples {

inline std::filesystem::path GetDefaultSampleDataDirectory(const std::filesystem::path& leafDirectory)
{
    std::filesystem::path directory = GetCurrentProcessExecutableDirectory();
    if (directory.empty()) {
        directory = std::filesystem::current_path();
    }

    directory /= "ImWidgetV4Data";
    if (!leafDirectory.empty()) {
        directory /= leafDirectory;
    }

    std::error_code errorCode;
    std::filesystem::create_directories(directory, errorCode);
    return directory;
}

inline std::filesystem::path GetDefaultSampleImGuiIniPath(const std::filesystem::path& fileName)
{
    return GetDefaultSampleDataDirectory("imgui") / fileName;
}

} // namespace ImWidgetV4::Samples



