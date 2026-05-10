#include "EnvironmentProbe.h"

#include <imwidgetv4/platform/PlatformProcess.h>

#include <algorithm>
#include <fstream>

namespace ImWidgetV4Editor {

namespace {

bool PathExists(const std::filesystem::path& path)
{
    return !path.empty() && std::filesystem::exists(path);
}

std::string CaptureFirstOutputLine(
    const std::filesystem::path& workingDirectory,
    const std::vector<std::string>& arguments,
    bool& outSuccess)
{
    std::string firstLine;
    const ImWidgetV4::FProcessExecutionResult result = ImWidgetV4::ExecuteProcess(
        workingDirectory,
        arguments,
        [&firstLine](const std::string& line) {
            if (firstLine.empty() && !line.empty()) {
                firstLine = line;
            }
        });
    outSuccess = result.bSuccess;
    return firstLine;
}

std::filesystem::path ResolveAndroidSdkRoot()
{
    std::filesystem::path sdkRoot = ImWidgetV4::GetEnvironmentPathVariable("ANDROID_SDK_ROOT");
    if (!sdkRoot.empty()) {
        return sdkRoot;
    }

    sdkRoot = ImWidgetV4::GetEnvironmentPathVariable("ANDROID_HOME");
    if (!sdkRoot.empty()) {
        return sdkRoot;
    }

    return {};
}

std::filesystem::path ResolveAndroidNdkRoot(const std::filesystem::path& sdkRoot)
{
    std::filesystem::path ndkRoot = ImWidgetV4::GetEnvironmentPathVariable("ANDROID_NDK_ROOT");
    if (!ndkRoot.empty()) {
        return ndkRoot;
    }

    ndkRoot = ImWidgetV4::GetEnvironmentPathVariable("NDK_ROOT");
    if (!ndkRoot.empty()) {
        return ndkRoot;
    }

    ndkRoot = ImWidgetV4::GetEnvironmentPathVariable("NDK_HOME");
    if (!ndkRoot.empty()) {
        return ndkRoot;
    }

    if (sdkRoot.empty()) {
        return {};
    }

    const std::filesystem::path bundlePath = sdkRoot / "ndk-bundle";
    if (PathExists(bundlePath)) {
        return bundlePath;
    }

    const std::filesystem::path ndkDirectory = sdkRoot / "ndk";
    if (!PathExists(ndkDirectory) || !std::filesystem::is_directory(ndkDirectory)) {
        return {};
    }

    std::vector<std::filesystem::path> candidates;
    for (const auto& entry : std::filesystem::directory_iterator(ndkDirectory)) {
        if (entry.is_directory()) {
            candidates.push_back(entry.path());
        }
    }

    if (candidates.empty()) {
        return {};
    }

    std::sort(candidates.begin(), candidates.end());
    return candidates.back();
}

FEnvironmentProbeItem BuildItem(
    const std::string& label,
    EEnvironmentProbeStatus status,
    const std::string& details)
{
    FEnvironmentProbeItem item;
    item.Label = label;
    item.Status = status;
    item.Details = details;
    return item;
}

void ProbeCommonTools(FEnvironmentProbeReport& report)
{
    bool bHasCMake = false;
    report.CMakeVersionLine = CaptureFirstOutputLine(
        std::filesystem::current_path(),
        {"cmake", "--version"},
        bHasCMake);
    report.Items.push_back(BuildItem(
        "CMake",
        bHasCMake ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Missing,
        bHasCMake
            ? (report.CMakeVersionLine.empty() ? "cmake available." : report.CMakeVersionLine)
            : "cmake was not found in PATH."));
}

void ProbeWindowsDesktop(FEnvironmentProbeReport& report)
{
    bool bHasCompiler = false;
    std::string compilerLine = CaptureFirstOutputLine(
        std::filesystem::current_path(),
        {"cl"},
        bHasCompiler);
    if (!bHasCompiler) {
        compilerLine = CaptureFirstOutputLine(
            std::filesystem::current_path(),
            {"clang++", "--version"},
            bHasCompiler);
    }
    if (!bHasCompiler) {
        compilerLine = CaptureFirstOutputLine(
            std::filesystem::current_path(),
            {"g++", "--version"},
            bHasCompiler);
    }

    report.Items.push_back(BuildItem(
        "Compiler",
        bHasCompiler ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Warning,
        bHasCompiler
            ? (compilerLine.empty() ? "Detected a C++ compiler in PATH." : compilerLine)
            : "No compiler probe succeeded. CMake may still work if a configured generator provides one."));    
}

void ProbeAndroid(FEnvironmentProbeReport& report, const FEditorBuildProfile& profile)
{
    report.AndroidSdkRoot = ResolveAndroidSdkRootForProfile(profile);
    report.AndroidNdkRoot = ResolveAndroidNdkRootForProfile(profile, report.AndroidSdkRoot);
    if (!report.AndroidNdkRoot.empty()) {
        report.AndroidToolchainFile =
            report.AndroidNdkRoot / "build" / "cmake" / "android.toolchain.cmake";
    }

    report.Items.push_back(BuildItem(
        "Android SDK",
        PathExists(report.AndroidSdkRoot) ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Missing,
        PathExists(report.AndroidSdkRoot)
            ? report.AndroidSdkRoot.string()
            : "Set ANDROID_SDK_ROOT or ANDROID_HOME."));
    report.Items.push_back(BuildItem(
        "Android NDK",
        PathExists(report.AndroidNdkRoot) ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Missing,
        PathExists(report.AndroidNdkRoot)
            ? report.AndroidNdkRoot.string()
            : "Set ANDROID_NDK_ROOT/NDK_ROOT or install an NDK under the Android SDK."));
    report.Items.push_back(BuildItem(
        "Android Toolchain",
        PathExists(report.AndroidToolchainFile) ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Missing,
        PathExists(report.AndroidToolchainFile)
            ? report.AndroidToolchainFile.string()
            : "Could not resolve android.toolchain.cmake from the detected NDK."));

    bool bHasJava = false;
    report.JavaVersionLine = CaptureFirstOutputLine(
        std::filesystem::current_path(),
        {"java", "-version"},
        bHasJava);
    if (!bHasJava) {
        const std::filesystem::path javaHome = ImWidgetV4::GetEnvironmentPathVariable("JAVA_HOME");
        if (PathExists(javaHome)) {
            bHasJava = true;
            report.JavaVersionLine = javaHome.string();
        }
    }
    report.Items.push_back(BuildItem(
        "Java",
        bHasJava ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Warning,
        bHasJava
            ? (report.JavaVersionLine.empty() ? "Java runtime detected." : report.JavaVersionLine)
            : "Java was not detected. Native CMake builds may work, but Gradle packaging will need Java."));    

    const bool bNeedsNinja = !profile.Generator.empty() && profile.Generator == "Ninja";
    bool bHasNinja = false;
    report.NinjaVersionLine = CaptureFirstOutputLine(
        std::filesystem::current_path(),
        {"ninja", "--version"},
        bHasNinja);
    report.Items.push_back(BuildItem(
        "Ninja",
        bNeedsNinja
            ? (bHasNinja ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Missing)
            : (bHasNinja ? EEnvironmentProbeStatus::Ok : EEnvironmentProbeStatus::Warning),
        bHasNinja
            ? (report.NinjaVersionLine.empty() ? "ninja available." : ("ninja " + report.NinjaVersionLine))
            : (bNeedsNinja
                ? "Selected Android build profile requires Ninja in PATH."
                : "Ninja not found. Configure a different generator if needed.")));
}

} // namespace

FEnvironmentProbeReport EnvironmentProbe::Probe(const FEditorBuildProfile& profile)
{
    FEnvironmentProbeReport report;
    report.TargetPlatform = profile.TargetPlatform;

    ProbeCommonTools(report);

    switch (profile.TargetPlatform) {
    case EEditorTargetPlatform::WindowsDesktop:
        ProbeWindowsDesktop(report);
        break;
    case EEditorTargetPlatform::Android:
        ProbeAndroid(report, profile);
        break;
    default:
        break;
    }

    report.bReady = std::all_of(
        report.Items.begin(),
        report.Items.end(),
        [](const FEnvironmentProbeItem& item) {
            return item.Status != EEnvironmentProbeStatus::Missing;
        });
    return report;
}

std::filesystem::path ResolveAndroidSdkRootForProfile(const FEditorBuildProfile& profile)
{
    if (!profile.AndroidSettings.SdkRootOverride.empty()) {
        return profile.AndroidSettings.SdkRootOverride.lexically_normal();
    }

    return ResolveAndroidSdkRoot();
}

std::filesystem::path ResolveAndroidNdkRootForProfile(
    const FEditorBuildProfile& profile,
    const std::filesystem::path& sdkRoot)
{
    if (!profile.AndroidSettings.NdkRootOverride.empty()) {
        return profile.AndroidSettings.NdkRootOverride.lexically_normal();
    }

    return ResolveAndroidNdkRoot(sdkRoot);
}

std::string ToDisplayString(EEnvironmentProbeStatus status)
{
    switch (status) {
    case EEnvironmentProbeStatus::Ok:
        return "OK";
    case EEnvironmentProbeStatus::Warning:
        return "WARN";
    case EEnvironmentProbeStatus::Missing:
        return "MISSING";
    default:
        return "UNKNOWN";
    }
}

} // namespace ImWidgetV4Editor
