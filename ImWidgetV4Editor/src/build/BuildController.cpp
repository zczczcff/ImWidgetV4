#include "BuildController.h"

#include <imwidgetv4/platform/PlatformProcess.h>
#include <algorithm>
#include <cctype>
#include <fstream>
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
    const FEditorBuildProfile& profile)
{
    if (settings.LibraryIntegrationMode != EEditorLibraryIntegrationMode::SDK ||
        settings.SdkPackagePath.empty() ||
        profile.Configuration.empty()) {
        return true;
    }

    const std::filesystem::path sdkPackagePath = settings.SdkPackagePath.is_absolute()
        ? settings.SdkPackagePath
        : (projectRoot / settings.SdkPackagePath).lexically_normal();
    const std::string architecture = profile.TargetPlatform == EEditorTargetPlatform::WindowsDesktop
        ? NormalizeWindowsArchitecture(profile.WindowsSettings.Architecture)
        : std::string();
    if (!architecture.empty()) {
        const std::filesystem::path architectureTargetsFile =
            sdkPackagePath / ("ImWidgetV4Targets-" + architecture + "-" + ToLowerCopy(profile.Configuration) + ".cmake");
        std::error_code errorCode;
        if (std::filesystem::is_regular_file(architectureTargetsFile, errorCode)) {
            return true;
        }
    }

    const std::filesystem::path targetsFile =
        sdkPackagePath / ("ImWidgetV4Targets-" + ToLowerCopy(profile.Configuration) + ".cmake");
    std::error_code errorCode;
    return std::filesystem::is_regular_file(targetsFile, errorCode);
}

std::string GetWindowsCMakePlatform(const FEditorBuildProfile& profile)
{
    return NormalizeWindowsArchitecture(profile.WindowsSettings.Architecture) == "win32" ? "Win32" : "x64";
}

std::string GetWindowsSdkArchitecture(const FEditorBuildProfile& profile)
{
    return NormalizeWindowsArchitecture(profile.WindowsSettings.Architecture);
}

std::string GetVisualStudioToolsetForGenerator(const std::string& generator)
{
    if (generator.find("Visual Studio 17") != std::string::npos) {
        return "v143";
    }
    if (generator.find("Visual Studio 16") != std::string::npos) {
        return "v142";
    }
    if (generator.find("Visual Studio 15") != std::string::npos) {
        return "v141";
    }
    return {};
}

std::string GetToolsetOverrideFromExtraArguments(const std::vector<std::string>& extraArguments)
{
    for (std::size_t index = 0; index < extraArguments.size(); ++index) {
        const std::string& argument = extraArguments[index];
        if (argument == "-T" && index + 1 < extraArguments.size()) {
            return extraArguments[index + 1];
        }
        const std::string toolsetPrefix = "-T";
        if (argument.rfind(toolsetPrefix, 0) == 0 && argument.size() > toolsetPrefix.size()) {
            return argument.substr(toolsetPrefix.size());
        }
    }
    return {};
}

std::string ReadCMakeSetValue(const std::filesystem::path& filePath, const std::string& variableName)
{
    std::ifstream stream(filePath, std::ios::binary);
    if (!stream) {
        return {};
    }

    const std::string prefix = "set(" + variableName + " \"";
    std::string line;
    while (std::getline(stream, line)) {
        const std::size_t begin = line.find(prefix);
        if (begin == std::string::npos) {
            continue;
        }
        const std::size_t valueBegin = begin + prefix.size();
        const std::size_t valueEnd = line.find('"', valueBegin);
        if (valueEnd == std::string::npos) {
            continue;
        }
        return line.substr(valueBegin, valueEnd - valueBegin);
    }

    return {};
}

bool ValidateSdkArchitectureAndToolset(
    const std::filesystem::path& projectRoot,
    const FEditorApplicationSettings& settings,
    const FEditorBuildProfile& profile,
    std::string* outError)
{
    if (settings.LibraryIntegrationMode != EEditorLibraryIntegrationMode::SDK ||
        profile.TargetPlatform != EEditorTargetPlatform::WindowsDesktop ||
        settings.SdkPackagePath.empty()) {
        return true;
    }

    const std::filesystem::path sdkPackagePath = settings.SdkPackagePath.is_absolute()
        ? settings.SdkPackagePath
        : (projectRoot / settings.SdkPackagePath).lexically_normal();
    const std::string architecture = GetWindowsSdkArchitecture(profile);
    const std::filesystem::path metadataPath =
        sdkPackagePath / ("ImWidgetV4SdkMetadata-" + architecture + ".cmake");
    if (!std::filesystem::is_regular_file(metadataPath)) {
        if (outError) {
            *outError =
                "The selected ImWidgetV4 SDK does not provide architecture '" + architecture + "'.";
        }
        return false;
    }

    const std::string compilerId = ReadCMakeSetValue(metadataPath, "ImWidgetV4_SDK_COMPILER_ID");
    const bool bUsesVisualStudioGenerator = profile.Generator.find("Visual Studio") != std::string::npos;
    if (!compilerId.empty() && bUsesVisualStudioGenerator && compilerId != "MSVC") {
        if (outError) {
            *outError =
                "The selected ImWidgetV4 SDK was built with compiler '" + compilerId +
                "', but this Windows build profile expects MSVC.";
        }
        return false;
    }

    const std::string sdkToolset = ReadCMakeSetValue(metadataPath, "ImWidgetV4_SDK_MSVC_TOOLSET");
    std::string expectedToolset = GetToolsetOverrideFromExtraArguments(profile.ExtraConfigureArguments);
    if (expectedToolset.empty()) {
        expectedToolset = GetVisualStudioToolsetForGenerator(profile.Generator);
    }
    if (!sdkToolset.empty() && !expectedToolset.empty() && sdkToolset != expectedToolset) {
        if (outError) {
            *outError =
                "The selected ImWidgetV4 SDK was built with MSVC toolset '" + sdkToolset +
                "', but the active build profile expects '" + expectedToolset + "'.";
        }
        return false;
    }

    return true;
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

    if (!HasSdkImportedConfiguration(project.GetProjectRoot(), project.GetApplicationSettings(), *profile)) {
        FBuildResult result;
        result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
        result.ErrorMessage =
            "The selected ImWidgetV4 SDK does not provide " + profile->Configuration +
            " libraries. Build the app with a matching SDK configuration, or package the SDK with that configuration.";
        return result;
    }

    std::string sdkCompatibilityError;
    if (!ValidateSdkArchitectureAndToolset(
            project.GetProjectRoot(),
            project.GetApplicationSettings(),
            *profile,
            &sdkCompatibilityError)) {
        FBuildResult result;
        result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
        result.ErrorMessage = sdkCompatibilityError;
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

    if (!HasSdkImportedConfiguration(project.GetProjectRoot(), project.GetApplicationSettings(), *profile)) {
        FBuildResult result;
        result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
        result.ErrorMessage =
            "The selected ImWidgetV4 SDK does not provide " + profile->Configuration +
            " libraries. Build the app with a matching SDK configuration, or package the SDK with that configuration.";
        return result;
    }

    std::string sdkCompatibilityError;
    if (!ValidateSdkArchitectureAndToolset(
            project.GetProjectRoot(),
            project.GetApplicationSettings(),
            *profile,
            &sdkCompatibilityError)) {
        FBuildResult result;
        result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), *profile);
        result.ErrorMessage = sdkCompatibilityError;
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

    if (profile.TargetPlatform == EEditorTargetPlatform::WindowsDesktop &&
        profile.Generator.find("Visual Studio") != std::string::npos) {
        arguments.push_back("-A");
        arguments.push_back(GetWindowsCMakePlatform(profile));
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
