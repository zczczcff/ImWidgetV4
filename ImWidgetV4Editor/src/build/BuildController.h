#pragma once

#include "../project/EditorProject.h"

#include <filesystem>
#include <functional>
#include <string>

namespace ImWidgetV4Editor {

struct FBuildResult {
    bool bSuccess = false;
    int ExitCode = -1;
    std::filesystem::path BuildDirectory;
    std::string ErrorMessage;
};

class BuildController {
public:
    using FOutputCallback = std::function<void(const std::string&)>;

    static std::filesystem::path GetDefaultBuildDirectory(
        const std::filesystem::path& projectRoot,
        const std::string& configuration = "Debug");
    static std::filesystem::path GetDefaultLibraryRoot();

    FBuildResult ConfigureProject(
        const EditorProject& project,
        const FOutputCallback& outputCallback,
        const std::string& configuration = "Debug") const;
    FBuildResult BuildProject(
        const EditorProject& project,
        const FOutputCallback& outputCallback,
        const std::string& configuration = "Debug") const;

private:
    static FBuildResult RunProcess(
        const std::filesystem::path& workingDirectory,
        const std::wstring& commandLine,
        const std::filesystem::path& buildDirectory,
        const FOutputCallback& outputCallback);
};

} // namespace ImWidgetV4Editor
