#include "BuildController.h"

#include <imwidgetv4/platform/PlatformProcess.h>
#include <algorithm>
#include <sstream>

namespace ImWidgetV4Editor {

namespace {

std::filesystem::path ResolveCompileTimeLibraryRoot()
{
    std::filesystem::path sourcePath = std::filesystem::path(__FILE__).lexically_normal();
    for (int index = 0; index < 3; ++index) {
        sourcePath = sourcePath.parent_path();
    }
    return (sourcePath.parent_path() / "ImWidgetV4").lexically_normal();
}

std::string ResolveHostPlatformTag()
{
#if defined(__ANDROID__)
    return "android";
#elif defined(_WIN32)
    return "win32";
#elif defined(__APPLE__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "native";
#endif
}

} // namespace

std::filesystem::path BuildController::GetDefaultBuildDirectory(
    const std::filesystem::path& projectRoot,
    const std::string& configuration)
{
    std::string normalizedConfiguration = configuration;
    std::transform(
        normalizedConfiguration.begin(),
        normalizedConfiguration.end(),
        normalizedConfiguration.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalizedConfiguration.empty()) {
        normalizedConfiguration = "debug";
    }

    return (projectRoot / "build" / (ResolveHostPlatformTag() + "-" + normalizedConfiguration)).lexically_normal();
}

std::filesystem::path BuildController::GetDefaultLibraryRoot()
{
    const std::filesystem::path environmentRoot = ImWidgetV4::GetEnvironmentPathVariable("IMWIDGETV4_ROOT");
    if (!environmentRoot.empty()) {
        return environmentRoot;
    }

    return ResolveCompileTimeLibraryRoot();
}

FBuildResult BuildController::ConfigureProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback,
    const std::string& configuration) const
{
    FBuildResult result;
    result.BuildDirectory = GetDefaultBuildDirectory(project.GetProjectRoot(), configuration);

    if (project.GetProjectRoot().empty()) {
        result.ErrorMessage = "Project root is empty.";
        return result;
    }

    const std::filesystem::path libraryRoot = GetDefaultLibraryRoot();
    std::vector<std::string> arguments = {
        "cmake",
        "-S", project.GetProjectRoot().string(),
        "-B", result.BuildDirectory.string(),
        "-DIMWIDGETV4_ROOT=" + libraryRoot.string()
    };

    if (outputCallback) {
        outputCallback("[configure] " + ImWidgetV4::BuildProcessCommandLineForDisplay(arguments));
    }
    return RunProcess(project.GetProjectRoot(), arguments, result.BuildDirectory, outputCallback);
}

FBuildResult BuildController::BuildProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback,
    const std::string& configuration) const
{
    FBuildResult configureResult;
    const std::filesystem::path buildDirectory = GetDefaultBuildDirectory(project.GetProjectRoot(), configuration);
    if (!std::filesystem::exists(buildDirectory / "CMakeCache.txt")) {
        configureResult = ConfigureProject(project, outputCallback, configuration);
        if (!configureResult.bSuccess) {
            return configureResult;
        }
    }

    std::vector<std::string> arguments = {
        "cmake",
        "--build", buildDirectory.string(),
        "--config", configuration
    };

    if (outputCallback) {
        outputCallback("[build] " + ImWidgetV4::BuildProcessCommandLineForDisplay(arguments));
    }
    return RunProcess(project.GetProjectRoot(), arguments, buildDirectory, outputCallback);
}

FBuildResult BuildController::RunProcess(
    const std::filesystem::path& workingDirectory,
    const std::vector<std::string>& arguments,
    const std::filesystem::path& buildDirectory,
    const FOutputCallback& outputCallback)
{
    FBuildResult result;
    result.BuildDirectory = buildDirectory;

    const ImWidgetV4::FProcessExecutionResult processResult =
        ImWidgetV4::ExecuteProcess(workingDirectory, arguments, outputCallback);
    result.bSuccess = processResult.bSuccess;
    result.ExitCode = processResult.ExitCode;
    result.ErrorMessage = processResult.ErrorMessage;
    return result;
}

} // namespace ImWidgetV4Editor
