#pragma once

#include "../serialization/DocumentFormat.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

enum class EEditorTargetPlatform {
    WindowsDesktop,
    Android
};

struct FAndroidBuildSettings {
    std::string Abi = "arm64-v8a";
    int ApiLevel = 24;
    std::string Stl = "c++_shared";
};

struct FEditorBuildProfile {
    std::string Name;
    EEditorTargetPlatform TargetPlatform = EEditorTargetPlatform::WindowsDesktop;
    std::string Configuration = "Debug";
    std::string Generator;
    std::filesystem::path BuildDirectory;
    FAndroidBuildSettings AndroidSettings;
    std::vector<std::string> ExtraConfigureArguments;
    bool bEnabled = true;
};

std::string GetTargetPlatformId(EEditorTargetPlatform platform);
std::string GetTargetPlatformDisplayName(EEditorTargetPlatform platform);
std::string GetTargetPlatformBuildTag(EEditorTargetPlatform platform);
bool TryParseTargetPlatform(const std::string& text, EEditorTargetPlatform& outPlatform);

std::filesystem::path BuildDefaultBuildDirectoryRelativePath(
    EEditorTargetPlatform platform,
    const std::string& configuration);
std::filesystem::path ResolveBuildDirectoryPath(
    const std::filesystem::path& projectRoot,
    const FEditorBuildProfile& profile);

std::vector<FEditorBuildProfile> BuildDefaultBuildProfiles();
const FEditorBuildProfile* FindBuildProfileByName(
    const std::vector<FEditorBuildProfile>& profiles,
    const std::string& profileName);
FEditorBuildProfile* FindBuildProfileByName(
    std::vector<FEditorBuildProfile>& profiles,
    const std::string& profileName);

json BuildProfileToJson(const FEditorBuildProfile& profile);
bool BuildProfileFromJson(const json& profileJson, FEditorBuildProfile& outProfile, std::string* outError = nullptr);

} // namespace ImWidgetV4Editor
