#include "PlatformConfiguration.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ImWidgetV4Editor {

namespace {

std::string NormalizeConfiguration(const std::string& configuration)
{
    std::string normalized = configuration;
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized.empty()) {
        normalized = "debug";
    }
    return normalized;
}

std::filesystem::path NormalizeStoredBuildDirectory(const std::filesystem::path& buildDirectory)
{
    if (buildDirectory.empty()) {
        return {};
    }

    return buildDirectory.lexically_normal();
}

} // namespace

std::string GetTargetPlatformId(EEditorTargetPlatform platform)
{
    switch (platform) {
    case EEditorTargetPlatform::WindowsDesktop:
        return "WindowsDesktop";
    case EEditorTargetPlatform::Android:
        return "Android";
    default:
        return "WindowsDesktop";
    }
}

std::string GetTargetPlatformDisplayName(EEditorTargetPlatform platform)
{
    switch (platform) {
    case EEditorTargetPlatform::WindowsDesktop:
        return "Windows Desktop";
    case EEditorTargetPlatform::Android:
        return "Android";
    default:
        return "Windows Desktop";
    }
}

std::string GetTargetPlatformBuildTag(EEditorTargetPlatform platform)
{
    switch (platform) {
    case EEditorTargetPlatform::WindowsDesktop:
        return "win32";
    case EEditorTargetPlatform::Android:
        return "android";
    default:
        return "native";
    }
}

std::string NormalizeWindowsArchitecture(const std::string& architecture)
{
    std::string normalized = architecture;
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "win32" || normalized == "x86") {
        return "win32";
    }
    return "win64";
}

bool TryParseTargetPlatform(const std::string& text, EEditorTargetPlatform& outPlatform)
{
    if (text == "WindowsDesktop" || text == "windows" || text == "win32") {
        outPlatform = EEditorTargetPlatform::WindowsDesktop;
        return true;
    }

    if (text == "Android" || text == "android") {
        outPlatform = EEditorTargetPlatform::Android;
        return true;
    }

    return false;
}

std::filesystem::path BuildDefaultBuildDirectoryRelativePath(
    EEditorTargetPlatform platform,
    const std::string& configuration)
{
    const std::string tag = platform == EEditorTargetPlatform::WindowsDesktop
        ? std::string("win64")
        : GetTargetPlatformBuildTag(platform);
    return (std::filesystem::path("build") / (tag + "-" + NormalizeConfiguration(configuration)))
        .lexically_normal();
}

std::filesystem::path BuildDefaultWindowsBuildDirectoryRelativePath(
    const std::string& architecture,
    const std::string& configuration)
{
    return (std::filesystem::path("build") /
        (NormalizeWindowsArchitecture(architecture) + "-" + NormalizeConfiguration(configuration)))
        .lexically_normal();
}

std::filesystem::path ResolveBuildDirectoryPath(
    const std::filesystem::path& projectRoot,
    const FEditorBuildProfile& profile)
{
    std::filesystem::path buildDirectory = profile.BuildDirectory;
    if (buildDirectory.empty()) {
        buildDirectory = BuildDefaultBuildDirectoryRelativePath(profile.TargetPlatform, profile.Configuration);
    }

    if (buildDirectory.is_absolute()) {
        return buildDirectory.lexically_normal();
    }

    return (projectRoot / buildDirectory).lexically_normal();
}

std::vector<FEditorBuildProfile> BuildDefaultBuildProfiles()
{
    FEditorBuildProfile windowsDebug;
    windowsDebug.Name = "Windows Debug";
    windowsDebug.TargetPlatform = EEditorTargetPlatform::WindowsDesktop;
    windowsDebug.Configuration = "Debug";
    windowsDebug.Generator = "Visual Studio 17 2022";
    windowsDebug.WindowsSettings.Architecture = "win64";
    windowsDebug.BuildDirectory = BuildDefaultWindowsBuildDirectoryRelativePath(
        windowsDebug.WindowsSettings.Architecture,
        windowsDebug.Configuration);

    FEditorBuildProfile windowsRelease;
    windowsRelease.Name = "Windows Release";
    windowsRelease.TargetPlatform = EEditorTargetPlatform::WindowsDesktop;
    windowsRelease.Configuration = "Release";
    windowsRelease.Generator = "Visual Studio 17 2022";
    windowsRelease.WindowsSettings.Architecture = "win64";
    windowsRelease.BuildDirectory = BuildDefaultWindowsBuildDirectoryRelativePath(
        windowsRelease.WindowsSettings.Architecture,
        windowsRelease.Configuration);

    FEditorBuildProfile androidDebug;
    androidDebug.Name = "Android Debug";
    androidDebug.TargetPlatform = EEditorTargetPlatform::Android;
    androidDebug.Configuration = "Debug";
    androidDebug.Generator = "Ninja";
    androidDebug.BuildDirectory = BuildDefaultBuildDirectoryRelativePath(
        androidDebug.TargetPlatform,
        androidDebug.Configuration);

    return {windowsDebug, windowsRelease, androidDebug};
}

const FEditorBuildProfile* FindBuildProfileByName(
    const std::vector<FEditorBuildProfile>& profiles,
    const std::string& profileName)
{
    const auto it = std::find_if(
        profiles.begin(),
        profiles.end(),
        [&profileName](const FEditorBuildProfile& profile) {
            return profile.Name == profileName;
        });
    return it == profiles.end() ? nullptr : &(*it);
}

FEditorBuildProfile* FindBuildProfileByName(
    std::vector<FEditorBuildProfile>& profiles,
    const std::string& profileName)
{
    const auto it = std::find_if(
        profiles.begin(),
        profiles.end(),
        [&profileName](const FEditorBuildProfile& profile) {
            return profile.Name == profileName;
        });
    return it == profiles.end() ? nullptr : &(*it);
}

json BuildProfileToJson(const FEditorBuildProfile& profile)
{
    json profileJson;
    profileJson["Name"] = profile.Name;
    profileJson["TargetPlatform"] = GetTargetPlatformId(profile.TargetPlatform);
    profileJson["Configuration"] = profile.Configuration;
    profileJson["Generator"] = profile.Generator;
    profileJson["BuildDirectory"] = profile.BuildDirectory.generic_string();
    profileJson["Enabled"] = profile.bEnabled;

    json windowsJson;
    windowsJson["Architecture"] = NormalizeWindowsArchitecture(profile.WindowsSettings.Architecture);
    profileJson["Windows"] = std::move(windowsJson);

    json androidJson;
    androidJson["Abi"] = profile.AndroidSettings.Abi;
    androidJson["ApiLevel"] = profile.AndroidSettings.ApiLevel;
    androidJson["Stl"] = profile.AndroidSettings.Stl;
    androidJson["SdkRootOverride"] = profile.AndroidSettings.SdkRootOverride.generic_string();
    androidJson["NdkRootOverride"] = profile.AndroidSettings.NdkRootOverride.generic_string();
    profileJson["Android"] = std::move(androidJson);

    profileJson["ExtraConfigureArguments"] = profile.ExtraConfigureArguments;
    return profileJson;
}

bool BuildProfileFromJson(const json& profileJson, FEditorBuildProfile& outProfile, std::string* outError)
{
    if (!profileJson.is_object()) {
        if (outError) {
            *outError = "Build profile entry must be an object.";
        }
        return false;
    }

    FEditorBuildProfile profile;
    profile.Name = profileJson.value("Name", std::string());
    if (profile.Name.empty()) {
        if (outError) {
            *outError = "Build profile is missing a name.";
        }
        return false;
    }

    EEditorTargetPlatform platform = EEditorTargetPlatform::WindowsDesktop;
    if (!TryParseTargetPlatform(profileJson.value("TargetPlatform", std::string()), platform)) {
        if (outError) {
            *outError = "Build profile has an unsupported target platform.";
        }
        return false;
    }

    profile.TargetPlatform = platform;
    profile.Configuration = profileJson.value("Configuration", std::string("Debug"));
    profile.Generator = profileJson.value("Generator", std::string());
    profile.BuildDirectory = NormalizeStoredBuildDirectory(
        std::filesystem::path(profileJson.value("BuildDirectory", std::string())));
    profile.bEnabled = profileJson.value("Enabled", true);

    const json windowsJson = profileJson.value("Windows", json::object());
    if (windowsJson.is_object()) {
        profile.WindowsSettings.Architecture =
            NormalizeWindowsArchitecture(windowsJson.value("Architecture", std::string("win64")));
    } else {
        profile.WindowsSettings.Architecture = "win64";
    }

    const json androidJson = profileJson.value("Android", json::object());
    if (androidJson.is_object()) {
        profile.AndroidSettings.Abi = androidJson.value("Abi", std::string("arm64-v8a"));
        profile.AndroidSettings.ApiLevel = androidJson.value("ApiLevel", 24);
        profile.AndroidSettings.Stl = androidJson.value("Stl", std::string("c++_shared"));
        profile.AndroidSettings.SdkRootOverride =
            NormalizeStoredBuildDirectory(std::filesystem::path(androidJson.value("SdkRootOverride", std::string())));
        profile.AndroidSettings.NdkRootOverride =
            NormalizeStoredBuildDirectory(std::filesystem::path(androidJson.value("NdkRootOverride", std::string())));
    }

    const json extraArgsJson = profileJson.value("ExtraConfigureArguments", json::array());
    if (extraArgsJson.is_array()) {
        for (const json& entry : extraArgsJson) {
            if (entry.is_string()) {
                profile.ExtraConfigureArguments.push_back(entry.get<std::string>());
            }
        }
    }

    outProfile = std::move(profile);
    return true;
}

} // namespace ImWidgetV4Editor
