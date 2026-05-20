#include "BuildController.h"

#include <imwidgetv4/platform/PlatformProcess.h>
#include <algorithm>
#include <sstream>

namespace ImWidgetV4Editor {

namespace {

std::string ToLowerCopy(std::string text)
{
    std::transform(
        text.begin(),
        text.end(),
        text.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return text;
}

std::filesystem::path ResolveCompileTimeLibraryRoot()
{
    std::filesystem::path sourcePath = std::filesystem::path(__FILE__).lexically_normal();
    for (int index = 0; index < 3; ++index) {
        sourcePath = sourcePath.parent_path();
    }
    return (sourcePath.parent_path() / "ImWidgetV4").lexically_normal();
}

bool HasSdkImportedConfiguration(
    const std::filesystem::path& projectRoot,
    const FEditorApplicationSettings& settings,
    const std::string& configuration)
{
    if (settings.LibraryIntegrationMode != EEditorLibraryIntegrationMode::SDK ||
        settings.SdkPackagePath.empty() ||
        configuration.empty()) {
        return true;
    }

    const std::filesystem::path sdkPackagePath = settings.SdkPackagePath.is_absolute()
        ? settings.SdkPackagePath
        : (projectRoot / settings.SdkPackagePath).lexically_normal();
    const std::filesystem::path targetsFile =
        sdkPackagePath / ("ImWidgetV4Targets-" + ToLowerCopy(configuration) + ".cmake");
    std::error_code errorCode;
    return std::filesystem::is_regular_file(targetsFile, errorCode);
}

} // namespace

std::filesystem::path BuildController::GetDefaultBuildDirectory(
    const std::filesystem::path& projectRoot,
    const std::string& configuration)
{
    return GetDefaultBuildDirectory(projectRoot, EEditorTargetPlatform::WindowsDesktop, configuration);
}

std::filesystem::path BuildController::GetDefaultBuildDirectory(
    const std::filesystem::path& projectRoot,
    EEditorTargetPlatform targetPlatform,
    const std::string& configuration)
{
    FEditorBuildProfile profile;
    profile.TargetPlatform = targetPlatform;
    profile.Configuration = configuration;
    profile.BuildDirectory = BuildDefaultBuildDirectoryRelativePath(targetPlatform, configuration);
    return ResolveBuildDirectoryPath(projectRoot, profile);
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
    const FOutputCallback& outputCallback) const
{
    const FEditorBuildProfile* activeProfile = project.GetActiveBuildProfile();
    if (activeProfile == nullptr) {
        return BuildMissingProfileResult(project.GetActiveBuildProfileName());
    }

    return ConfigureProject(project, activeProfile->Name, outputCallback);
}

FBuildResult BuildController::BuildProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback) const
{
    const FEditorBuildProfile* activeProfile = project.GetActiveBuildProfile();
    if (activeProfile == nullptr) {
        return BuildMissingProfileResult(project.GetActiveBuildProfileName());
    }

    return BuildProject(project, activeProfile->Name, outputCallback);
}

FBuildResult BuildController::CleanProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback) const
{
    const FEditorBuildProfile* activeProfile = project.GetActiveBuildProfile();
    if (activeProfile == nullptr) {
        return BuildMissingProfileResult(project.GetActiveBuildProfileName());
    }

    return CleanProject(project, activeProfile->Name, outputCallback);
}

FBuildResult BuildController::RebuildProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback) const
{
    const FEditorBuildProfile* activeProfile = project.GetActiveBuildProfile();
    if (activeProfile == nullptr) {
        return BuildMissingProfileResult(project.GetActiveBuildProfileName());
    }

    return RebuildProject(project, activeProfile->Name, outputCallback);
}

FBuildResult BuildController::ConfigureProject(
    const EditorProject& project,
    const std::string& profileName,
    const FOutputCallback& outputCallback) const
{
    FBuildResult result;

    if (project.GetProjectRoot().empty()) {
        result.ErrorMessage = "Project root is empty.";
        return result;
    }

    const FEditorBuildProfile* profile = project.FindBuildProfile(profileName);
    if (profile == nullptr) {
        return BuildMissingProfileResult(profileName);
    }

    if (!HasSdkImportedConfiguration(project.GetProjectRoot(), project.GetApplicationSettings(), profile->Configuration)) {
        FBuildResult result;
        result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
        result.ErrorMessage =
            "The selected ImWidgetV4 SDK does not provide " + profile->Configuration +
            " libraries. Build the app with a matching SDK configuration, or package the SDK with that configuration.";
        return result;
    }

    result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);

    const FEnvironmentProbeReport probeReport = EnvironmentProbe::Probe(*profile);
    EmitEnvironmentProbeReport(probeReport, outputCallback);
    if (!probeReport.bReady) {
        return BuildEnvironmentFailureResult(*profile, project.GetProjectRoot(), probeReport);
    }

    std::vector<std::string> arguments = BuildConfigureArguments(project, *profile, probeReport);

    if (outputCallback) {
        outputCallback(
            "[configure:" + profile->Name + "] " +
            ImWidgetV4::BuildProcessCommandLineForDisplay(arguments));
    }
    return RunProcess(project.GetProjectRoot(), arguments, result.BuildDirectory, outputCallback);
}

FBuildResult BuildController::BuildProject(
    const EditorProject& project,
    const std::string& profileName,
    const FOutputCallback& outputCallback) const
{
    const FEditorBuildProfile* profile = project.FindBuildProfile(profileName);
    if (profile == nullptr) {
        return BuildMissingProfileResult(profileName);
    }

    if (!HasSdkImportedConfiguration(project.GetProjectRoot(), project.GetApplicationSettings(), profile->Configuration)) {
        FBuildResult result;
        result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
        result.ErrorMessage =
            "The selected ImWidgetV4 SDK does not provide " + profile->Configuration +
            " libraries. Build the app with a matching SDK configuration, or package the SDK with that configuration.";
        return result;
    }

    FBuildResult configureResult;
    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
    if (!std::filesystem::exists(buildDirectory / "CMakeCache.txt")) {
        configureResult = ConfigureProject(project, profile->Name, outputCallback);
        if (!configureResult.bSuccess) {
            return configureResult;
        }
    }

    std::vector<std::string> arguments = {
        "cmake",
        "--build", buildDirectory.string(),
        "--config", profile->Configuration
    };

    if (outputCallback) {
        outputCallback(
            "[build:" + profile->Name + "] " +
            ImWidgetV4::BuildProcessCommandLineForDisplay(arguments));
    }
    return RunProcess(project.GetProjectRoot(), arguments, buildDirectory, outputCallback);
}

FBuildResult BuildController::CleanProject(
    const EditorProject& project,
    const std::string& profileName,
    const FOutputCallback& outputCallback) const
{
    const FEditorBuildProfile* profile = project.FindBuildProfile(profileName);
    if (profile == nullptr) {
        return BuildMissingProfileResult(profileName);
    }

    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
    std::vector<std::string> arguments = {
        "cmake",
        "--build", buildDirectory.string(),
        "--config", profile->Configuration,
        "--target", "clean"
    };

    if (outputCallback) {
        outputCallback(
            "[clean:" + profile->Name + "] " +
            ImWidgetV4::BuildProcessCommandLineForDisplay(arguments));
    }
    return RunProcess(project.GetProjectRoot(), arguments, buildDirectory, outputCallback);
}

FBuildResult BuildController::RebuildProject(
    const EditorProject& project,
    const std::string& profileName,
    const FOutputCallback& outputCallback) const
{
    const FBuildResult cleanResult = CleanProject(project, profileName, outputCallback);
    if (!cleanResult.bSuccess) {
        return cleanResult;
    }

    return BuildProject(project, profileName, outputCallback);
}

std::vector<std::string> BuildController::BuildConfigureArguments(
    const EditorProject& project,
    const FEditorBuildProfile& profile,
    const FEnvironmentProbeReport& probeReport)
{
    const std::filesystem::path libraryRoot = GetDefaultLibraryRoot();
    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), profile);
    std::vector<std::string> arguments = {
        "cmake",
        "-S", project.GetProjectRoot().string(),
        "-B", buildDirectory.string()
    };

    if (project.GetApplicationSettings().LibraryIntegrationMode != EEditorLibraryIntegrationMode::SDK) {
        arguments.push_back("-DIMWIDGETV4_ROOT=" + libraryRoot.string());
    }

    if (!profile.Generator.empty()) {
        arguments.push_back("-G");
        arguments.push_back(profile.Generator);
    }

    if (profile.TargetPlatform == EEditorTargetPlatform::Android) {
        arguments.push_back("-DCMAKE_TOOLCHAIN_FILE=" + probeReport.AndroidToolchainFile.string());
        arguments.push_back("-DANDROID_ABI=" + profile.AndroidSettings.Abi);
        arguments.push_back("-DANDROID_PLATFORM=android-" + std::to_string(profile.AndroidSettings.ApiLevel));
        arguments.push_back("-DANDROID_STL=" + profile.AndroidSettings.Stl);
        if (!probeReport.AndroidSdkRoot.empty()) {
            arguments.push_back("-DANDROID_SDK_ROOT=" + probeReport.AndroidSdkRoot.string());
        }
        if (!probeReport.AndroidNdkRoot.empty()) {
            arguments.push_back("-DANDROID_NDK=" + probeReport.AndroidNdkRoot.string());
        }
    }

    for (const std::string& extraArgument : profile.ExtraConfigureArguments) {
        if (!extraArgument.empty()) {
            arguments.push_back(extraArgument);
        }
    }

    return arguments;
}

void BuildController::EmitEnvironmentProbeReport(
    const FEnvironmentProbeReport& report,
    const FOutputCallback& outputCallback)
{
    if (!outputCallback) {
        return;
    }

    outputCallback("[probe] Target: " + GetTargetPlatformDisplayName(report.TargetPlatform));
    for (const FEnvironmentProbeItem& item : report.Items) {
        outputCallback("[probe] " + item.Label + ": " + ToDisplayString(item.Status) + " - " + item.Details);
    }
}

FBuildResult BuildController::BuildMissingProfileResult(const std::string& profileName)
{
    FBuildResult result;
    result.ErrorMessage = profileName.empty()
        ? "No active build profile is configured."
        : "Build profile was not found: " + profileName;
    return result;
}

FBuildResult BuildController::BuildEnvironmentFailureResult(
    const FEditorBuildProfile& profile,
    const std::filesystem::path& projectRoot,
    const FEnvironmentProbeReport& report)
{
    FBuildResult result;
    result.BuildDirectory = ResolveBuildDirectoryPath(projectRoot, profile);
    result.ErrorMessage =
        "Environment probe failed for " + GetTargetPlatformDisplayName(report.TargetPlatform) + ".";
    result.ExitCode = -1;
    result.bSuccess = false;
    return result;
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
