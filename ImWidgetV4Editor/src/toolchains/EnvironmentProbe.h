#pragma once

#include "PlatformConfiguration.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

enum class EEnvironmentProbeStatus {
    Ok,
    Warning,
    Missing
};

struct FEnvironmentProbeItem {
    std::string Label;
    EEnvironmentProbeStatus Status = EEnvironmentProbeStatus::Missing;
    std::string Details;
};

struct FEnvironmentProbeReport {
    EEditorTargetPlatform TargetPlatform = EEditorTargetPlatform::WindowsDesktop;
    bool bReady = false;
    std::filesystem::path AndroidSdkRoot;
    std::filesystem::path AndroidNdkRoot;
    std::filesystem::path AndroidToolchainFile;
    std::string CMakeVersionLine;
    std::string NinjaVersionLine;
    std::string JavaVersionLine;
    std::vector<FEnvironmentProbeItem> Items;
};

class EnvironmentProbe {
public:
    static FEnvironmentProbeReport Probe(const FEditorBuildProfile& profile);
};

std::filesystem::path ResolveAndroidSdkRootForProfile(const FEditorBuildProfile& profile);
std::filesystem::path ResolveAndroidNdkRootForProfile(
    const FEditorBuildProfile& profile,
    const std::filesystem::path& sdkRoot);

std::string ToDisplayString(EEnvironmentProbeStatus status);

} // namespace ImWidgetV4Editor
