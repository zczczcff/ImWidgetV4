#pragma once

#include "../project/EditorProject.h"
#include "../toolchains\EnvironmentProbe.h"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

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
    static std::filesystem::path GetDefaultBuildDirectory(
        const std::filesystem::path& projectRoot,
        EEditorTargetPlatform targetPlatform,
        const std::string& configuration = "Debug");
    static std::filesystem::path GetDefaultLibraryRoot();

    FBuildResult ConfigureProject(
        const EditorProject& project,
        const FOutputCallback& outputCallback) const;
    FBuildResult BuildProject(
        const EditorProject& project,
        const FOutputCallback& outputCallback) const;
    FBuildResult CleanProject(
        const EditorProject& project,
        const FOutputCallback& outputCallback) const;
    FBuildResult RebuildProject(
        const EditorProject& project,
        const FOutputCallback& outputCallback) const;
    FBuildResult ConfigureProject(
        const EditorProject& project,
        const std::string& profileName,
        const FOutputCallback& outputCallback) const;
    FBuildResult BuildProject(
        const EditorProject& project,
        const std::string& profileName,
        const FOutputCallback& outputCallback) const;
    FBuildResult CleanProject(
        const EditorProject& project,
        const std::string& profileName,
        const FOutputCallback& outputCallback) const;
    FBuildResult RebuildProject(
        const EditorProject& project,
        const std::string& profileName,
        const FOutputCallback& outputCallback) const;
    static std::vector<std::string> BuildConfigureArguments(
        const EditorProject& project,
        const FEditorBuildProfile& profile,
        const FEnvironmentProbeReport& probeReport);

private:
    static void EmitEnvironmentProbeReport(
        const FEnvironmentProbeReport& report,
        const FOutputCallback& outputCallback);
    static FBuildResult BuildMissingProfileResult(const std::string& profileName);
    static FBuildResult BuildEnvironmentFailureResult(
        const FEditorBuildProfile& profile,
        const std::filesystem::path& projectRoot,
        const FEnvironmentProbeReport& report);
    static FBuildResult RunProcess(
        const std::filesystem::path& workingDirectory,
        const std::vector<std::string>& arguments,
        const std::filesystem::path& buildDirectory,
        const FOutputCallback& outputCallback);
};

} // namespace ImWidgetV4Editor
