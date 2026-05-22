#include "EditorProject.h"

#include <algorithm>
#include <cctype>
#include <fstream>

namespace ImWidgetV4Editor {

namespace {

std::filesystem::path NormalizeStoredRelativePath(const std::filesystem::path& relativePath)
{
    if (relativePath.empty()) {
        return {};
    }

    return relativePath.lexically_normal();
}

std::filesystem::path NormalizeStoredPath(const std::filesystem::path& path)
{
    if (path.empty()) {
        return {};
    }

    return path.lexically_normal();
}

std::string LibraryIntegrationModeToString(EEditorLibraryIntegrationMode mode)
{
    switch (mode) {
    case EEditorLibraryIntegrationMode::SDK:
        return "SDK";
    case EEditorLibraryIntegrationMode::Source:
    default:
        return "Source";
    }
}

EEditorLibraryIntegrationMode LibraryIntegrationModeFromString(const std::string& mode)
{
    return mode == "SDK"
        ? EEditorLibraryIntegrationMode::SDK
        : EEditorLibraryIntegrationMode::Source;
}

std::string ApplicationIconSourceToString(EEditorApplicationIconSource source)
{
    switch (source) {
    case EEditorApplicationIconSource::File:
        return "File";
    case EEditorApplicationIconSource::InternalCoreIcon:
        return "InternalCoreIcon";
    case EEditorApplicationIconSource::None:
    default:
        return "None";
    }
}

EEditorApplicationIconSource ApplicationIconSourceFromString(const std::string& source)
{
    if (source == "File") {
        return EEditorApplicationIconSource::File;
    }
    if (source == "InternalCoreIcon" || source == "Internal") {
        return EEditorApplicationIconSource::InternalCoreIcon;
    }
    return EEditorApplicationIconSource::None;
}

json ColorToJson(const ImWidgetV4::FColor& color)
{
    return json::array({
        static_cast<int>(std::clamp(color.R, 0.0f, 1.0f) * 255.0f + 0.5f),
        static_cast<int>(std::clamp(color.G, 0.0f, 1.0f) * 255.0f + 0.5f),
        static_cast<int>(std::clamp(color.B, 0.0f, 1.0f) * 255.0f + 0.5f),
        static_cast<int>(std::clamp(color.A, 0.0f, 1.0f) * 255.0f + 0.5f)
    });
}

ImWidgetV4::FColor ColorFromJson(const json& colorJson, const ImWidgetV4::FColor& fallback)
{
    if (!colorJson.is_array() || colorJson.size() < 3) {
        return fallback;
    }

    const auto readChannel = [&colorJson](std::size_t index, int fallbackValue) {
        if (index >= colorJson.size() || !colorJson[index].is_number_integer()) {
            return fallbackValue;
        }
        return std::clamp(colorJson[index].get<int>(), 0, 255);
    };
    const int r = readChannel(0, 255);
    const int g = readChannel(1, 255);
    const int b = readChannel(2, 255);
    const int a = readChannel(3, 255);
    return ImWidgetV4::FColor::FromBytes(r, g, b, a);
}

std::string NormalizeSdkVersion(const std::string& version)
{
    const auto first = std::find_if_not(version.begin(), version.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    });
    const auto last = std::find_if_not(version.rbegin(), version.rend(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }).base();
    if (first >= last) {
        return "0.1.0";
    }
    return std::string(first, last);
}

FEditorApplicationSettings BuildDefaultApplicationSettings(const std::string& projectName = std::string())
{
    FEditorApplicationSettings settings;
    settings.Title = projectName;
    settings.IconSource = EEditorApplicationIconSource::None;
    settings.IconPath.clear();
    settings.InternalIconName = "Package";
    settings.IconTint = ImWidgetV4::FColor::White;
    settings.IconBackground = ImWidgetV4::FColor::FromBytes(0, 0, 0, 0);
    settings.InitialWidth = 1280;
    settings.InitialHeight = 720;
    settings.bEnableIniSettings = false;
    settings.IniSettingsPath.clear();
    settings.bUseCustomHostChrome = false;
    settings.bUseTitleBar = false;
    settings.TitleBarDocumentRelativePath = std::filesystem::path("ui") / "TitleBar.ui.json";
    settings.bShowSystemButtons = true;
    settings.bUseTitleBarMenus = false;
    settings.LibraryIntegrationMode = EEditorLibraryIntegrationMode::Source;
    settings.SdkPackagePath.clear();
    settings.MinimumSdkVersion = "0.1.0";
    settings.DefaultTheme.clear();
    settings.DefaultCulture.clear();
    settings.StringTablePaths.clear();
    settings.bGenerateInitializeStub = false;
    settings.bGenerateTickStub = false;
    return settings;
}

json ApplicationSettingsToJson(const FEditorApplicationSettings& settings)
{
    json settingsJson;
    settingsJson["Title"] = settings.Title;
    settingsJson["IconSource"] = ApplicationIconSourceToString(settings.IconSource);
    settingsJson["IconPath"] = NormalizeStoredRelativePath(settings.IconPath).generic_string();
    settingsJson["InternalIcon"] = settings.InternalIconName;
    settingsJson["IconTint"] = ColorToJson(settings.IconTint);
    settingsJson["IconBackground"] = ColorToJson(settings.IconBackground);
    settingsJson["InitialWidth"] = settings.InitialWidth;
    settingsJson["InitialHeight"] = settings.InitialHeight;
    settingsJson["EnableIniSettings"] = settings.bEnableIniSettings;
    settingsJson["IniSettingsPath"] = NormalizeStoredRelativePath(settings.IniSettingsPath).generic_string();
    settingsJson["UseCustomHostChrome"] = settings.bUseCustomHostChrome;
    settingsJson["UseTitleBar"] = settings.bUseTitleBar;
    settingsJson["TitleBarDocument"] = NormalizeStoredRelativePath(settings.TitleBarDocumentRelativePath).generic_string();
    settingsJson["ShowSystemButtons"] = settings.bShowSystemButtons;
    settingsJson["UseTitleBarMenus"] = settings.bUseTitleBarMenus;
    settingsJson["LibraryIntegrationMode"] = LibraryIntegrationModeToString(settings.LibraryIntegrationMode);
    settingsJson["SdkPackagePath"] = NormalizeStoredPath(settings.SdkPackagePath).generic_string();
    settingsJson["MinimumSdkVersion"] = NormalizeSdkVersion(settings.MinimumSdkVersion);
    settingsJson["DefaultTheme"] = settings.DefaultTheme;
    settingsJson["DefaultCulture"] = settings.DefaultCulture;
    json stringTablePathsJson = json::array();
    for (const std::filesystem::path& path : settings.StringTablePaths) {
        const std::filesystem::path normalizedPath = NormalizeStoredRelativePath(path);
        if (!normalizedPath.empty() && !normalizedPath.is_absolute()) {
            stringTablePathsJson.push_back(normalizedPath.generic_string());
        }
    }
    settingsJson["StringTablePaths"] = std::move(stringTablePathsJson);
    settingsJson["GenerateInitializeStub"] = settings.bGenerateInitializeStub;
    settingsJson["GenerateTickStub"] = settings.bGenerateTickStub;
    return settingsJson;
}

FEditorApplicationSettings ApplicationSettingsFromJson(
    const json& settingsJson,
    const std::string& projectName)
{
    FEditorApplicationSettings settings = BuildDefaultApplicationSettings(projectName);
    if (!settingsJson.is_object()) {
        return settings;
    }

    settings.Title = settingsJson.value("Title", settings.Title);
    settings.IconSource = ApplicationIconSourceFromString(
        settingsJson.value("IconSource", std::string()));
    settings.IconPath = NormalizeStoredRelativePath(
        std::filesystem::path(settingsJson.value("IconPath", std::string())));
    if (settings.IconPath.is_absolute()) {
        settings.IconPath.clear();
    }
    if (settings.IconSource == EEditorApplicationIconSource::None && !settings.IconPath.empty()) {
        settings.IconSource = EEditorApplicationIconSource::File;
    }
    settings.InternalIconName = settingsJson.value("InternalIcon", settings.InternalIconName);
    settings.IconTint = ColorFromJson(settingsJson.value("IconTint", json()), settings.IconTint);
    settings.IconBackground = ColorFromJson(settingsJson.value("IconBackground", json()), settings.IconBackground);
    settings.InitialWidth = std::max(1, settingsJson.value("InitialWidth", settings.InitialWidth));
    settings.InitialHeight = std::max(1, settingsJson.value("InitialHeight", settings.InitialHeight));
    settings.bEnableIniSettings = settingsJson.value("EnableIniSettings", settings.bEnableIniSettings);
    settings.IniSettingsPath = NormalizeStoredRelativePath(
        std::filesystem::path(settingsJson.value("IniSettingsPath", std::string())));
    if (settings.IniSettingsPath.is_absolute()) {
        settings.IniSettingsPath.clear();
    }
    settings.bUseCustomHostChrome = settingsJson.value("UseCustomHostChrome", settings.bUseCustomHostChrome);
    settings.bUseTitleBar = settingsJson.value("UseTitleBar", settings.bUseTitleBar);
    settings.TitleBarDocumentRelativePath = NormalizeStoredRelativePath(
        std::filesystem::path(settingsJson.value("TitleBarDocument", settings.TitleBarDocumentRelativePath.generic_string())));
    if (settings.TitleBarDocumentRelativePath.empty() || settings.TitleBarDocumentRelativePath.is_absolute()) {
        settings.TitleBarDocumentRelativePath = std::filesystem::path("ui") / "TitleBar.ui.json";
    }
    settings.bShowSystemButtons = settingsJson.value("ShowSystemButtons", settings.bShowSystemButtons);
    settings.bUseTitleBarMenus = settingsJson.value("UseTitleBarMenus", settings.bUseTitleBarMenus);
    settings.LibraryIntegrationMode =
        LibraryIntegrationModeFromString(settingsJson.value("LibraryIntegrationMode", std::string("Source")));
    settings.SdkPackagePath = NormalizeStoredPath(
        std::filesystem::path(settingsJson.value("SdkPackagePath", std::string())));
    settings.MinimumSdkVersion = NormalizeSdkVersion(
        settingsJson.value("MinimumSdkVersion", settings.MinimumSdkVersion));
    settings.DefaultTheme = settingsJson.value("DefaultTheme", settings.DefaultTheme);
    settings.DefaultCulture = settingsJson.value("DefaultCulture", settings.DefaultCulture);
    const json stringTablePathsJson = settingsJson.value("StringTablePaths", json::array());
    if (stringTablePathsJson.is_array()) {
        for (const json& pathJson : stringTablePathsJson) {
            if (!pathJson.is_string()) {
                continue;
            }
            std::filesystem::path path = NormalizeStoredRelativePath(std::filesystem::path(pathJson.get<std::string>()));
            if (!path.empty() && !path.is_absolute()) {
                settings.StringTablePaths.push_back(std::move(path));
            }
        }
    }
    settings.bGenerateInitializeStub = settingsJson.value("GenerateInitializeStub", settings.bGenerateInitializeStub);
    settings.bGenerateTickStub = settingsJson.value("GenerateTickStub", settings.bGenerateTickStub);
    return settings;
}

} // namespace

std::string EditorProject::GetManifestFileName()
{
    return "imwidgetv4.project.json";
}

std::filesystem::path EditorProject::BuildManifestFilePath(const std::filesystem::path& projectRoot)
{
    return projectRoot / GetManifestFileName();
}

void EditorProject::Reset()
{
    m_ProjectName.clear();
    m_NamespaceName.clear();
    m_TemplateName = "Blank App";
    m_ProjectRoot.clear();
    m_StartupDocumentRelativePath.clear();
    m_ApplicationSettings = BuildDefaultApplicationSettings();
    m_BuildProfiles.clear();
    m_ActiveBuildProfileName.clear();
}

bool EditorProject::CreateNew(
    const std::filesystem::path& projectRoot,
    const std::string& projectName,
    const std::string& namespaceName,
    const std::filesystem::path& startupDocumentRelativePath,
    const std::string& templateName)
{
    Reset();
    m_ProjectRoot = projectRoot.lexically_normal();
    m_ProjectName = projectName;
    m_NamespaceName = namespaceName;
    m_TemplateName = templateName.empty() ? std::string("Blank App") : templateName;
    m_StartupDocumentRelativePath = NormalizeStoredRelativePath(startupDocumentRelativePath);
    m_ApplicationSettings = BuildDefaultApplicationSettings(m_ProjectName);
    EnsureBuildProfiles();
    return IsValid();
}

bool EditorProject::Load(const std::filesystem::path& manifestFilePath, std::string* outError)
{
    try {
        std::ifstream stream(manifestFilePath, std::ios::binary);
        if (!stream.is_open()) {
            if (outError) {
                *outError = "Failed to open project manifest for reading.";
            }
            return false;
        }

        json projectJson;
        stream >> projectJson;
        return FromJson(projectJson, manifestFilePath, outError);
    } catch (const std::exception& error) {
        if (outError) {
            *outError = error.what();
        }
        return false;
    } catch (...) {
        if (outError) {
            *outError = "Unknown project manifest load error.";
        }
        return false;
    }
}

bool EditorProject::Save(std::string* outError) const
{
    try {
        if (!IsValid()) {
            if (outError) {
                *outError = "Project metadata is incomplete.";
            }
            return false;
        }

        const std::filesystem::path manifestFilePath = GetManifestFilePath();
        const std::filesystem::path parentPath = manifestFilePath.parent_path();
        if (!parentPath.empty()) {
            std::error_code error;
            std::filesystem::create_directories(parentPath, error);
            if (error) {
                if (outError) {
                    *outError = "Failed to create project manifest directory.";
                }
                return false;
            }
        }

        std::ofstream stream(manifestFilePath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            if (outError) {
                *outError = "Failed to open project manifest for writing.";
            }
            return false;
        }

        stream << ToJson().dump(2);
        stream.flush();
        if (!stream.good()) {
            if (outError) {
                *outError = "Failed to write project manifest.";
            }
            return false;
        }

        return true;
    } catch (const std::exception& error) {
        if (outError) {
            *outError = error.what();
        }
        return false;
    } catch (...) {
        if (outError) {
            *outError = "Unknown project manifest save error.";
        }
        return false;
    }
}

json EditorProject::ToJson() const
{
    json projectJson;
    projectJson["Format"] = "ImWidgetV4EditorProject";
    projectJson["Version"] = FormatVersion;

    json projectSection;
    projectSection["Name"] = m_ProjectName;
    projectSection["Namespace"] = m_NamespaceName;
    projectSection["Template"] = m_TemplateName;
    projectSection["StartupDocument"] = m_StartupDocumentRelativePath.generic_string();
    projectSection["ActiveBuildProfile"] = m_ActiveBuildProfileName;
    projectJson["Project"] = std::move(projectSection);
    projectJson["Application"] = ApplicationSettingsToJson(m_ApplicationSettings);

    json buildProfilesJson = json::array();
    for (const FEditorBuildProfile& profile : m_BuildProfiles) {
        buildProfilesJson.push_back(BuildProfileToJson(profile));
    }
    projectJson["BuildProfiles"] = std::move(buildProfilesJson);
    return projectJson;
}

bool EditorProject::FromJson(
    const json& projectJson,
    const std::filesystem::path& manifestFilePath,
    std::string* outError)
{
    if (!projectJson.is_object()) {
        if (outError) {
            *outError = "Project manifest root must be an object.";
        }
        return false;
    }

    if (projectJson.value("Format", std::string()) != "ImWidgetV4EditorProject") {
        if (outError) {
            *outError = "Unsupported project manifest format.";
        }
        return false;
    }

    const int version = projectJson.value("Version", 0);
    if (version != 1 && version != 2 && version != 4 && version != 5 && version != FormatVersion) {
        if (outError) {
            *outError = "Unsupported project manifest version.";
        }
        return false;
    }

    const json projectSection = projectJson.value("Project", json::object());
    if (!projectSection.is_object()) {
        if (outError) {
            *outError = "Project manifest missing Project section.";
        }
        return false;
    }

    Reset();
    m_ProjectRoot = manifestFilePath.parent_path().lexically_normal();
    m_ProjectName = projectSection.value("Name", std::string());
    m_NamespaceName = projectSection.value("Namespace", std::string());
    m_TemplateName = projectSection.value("Template", std::string("Blank App"));
    m_StartupDocumentRelativePath = NormalizeStoredRelativePath(
        std::filesystem::path(projectSection.value("StartupDocument", std::string())));
    m_ActiveBuildProfileName = projectSection.value("ActiveBuildProfile", std::string());
    m_ApplicationSettings = ApplicationSettingsFromJson(
        projectJson.value("Application", json::object()),
        m_ProjectName);

    const json buildProfilesJson = projectJson.value("BuildProfiles", json::array());
    if (buildProfilesJson.is_array()) {
        for (const json& buildProfileJson : buildProfilesJson) {
            FEditorBuildProfile profile;
            std::string profileError;
            if (!BuildProfileFromJson(buildProfileJson, profile, &profileError)) {
                if (outError) {
                    *outError = profileError;
                }
                Reset();
                return false;
            }
            m_BuildProfiles.push_back(std::move(profile));
        }
    }

    EnsureBuildProfiles();

    if (!IsValid()) {
        if (outError) {
            *outError = "Project manifest is missing required fields.";
        }
        Reset();
        return false;
    }

    return true;
}

bool EditorProject::IsValid() const
{
    return !m_ProjectName.empty() &&
           !m_NamespaceName.empty() &&
           !m_TemplateName.empty() &&
           !m_ProjectRoot.empty() &&
           !m_StartupDocumentRelativePath.empty() &&
           !m_StartupDocumentRelativePath.is_absolute() &&
           !m_BuildProfiles.empty() &&
           !m_ActiveBuildProfileName.empty();
}

std::filesystem::path EditorProject::GetManifestFilePath() const
{
    return BuildManifestFilePath(m_ProjectRoot);
}

std::filesystem::path EditorProject::GetStartupDocumentPath() const
{
    if (m_ProjectRoot.empty() || m_StartupDocumentRelativePath.empty()) {
        return {};
    }

    return (m_ProjectRoot / m_StartupDocumentRelativePath).lexically_normal();
}

std::filesystem::path EditorProject::GetTitleBarDocumentPath() const
{
    if (m_ProjectRoot.empty() || m_ApplicationSettings.TitleBarDocumentRelativePath.empty()) {
        return {};
    }

    return (m_ProjectRoot / m_ApplicationSettings.TitleBarDocumentRelativePath).lexically_normal();
}

bool EditorProject::SetActiveBuildProfileName(const std::string& profileName)
{
    if (profileName.empty() || FindBuildProfile(profileName) == nullptr) {
        return false;
    }

    m_ActiveBuildProfileName = profileName;
    return true;
}

const FEditorBuildProfile* EditorProject::FindBuildProfile(const std::string& profileName) const
{
    return FindBuildProfileByName(m_BuildProfiles, profileName);
}

FEditorBuildProfile* EditorProject::FindBuildProfile(const std::string& profileName)
{
    return FindBuildProfileByName(m_BuildProfiles, profileName);
}

const FEditorBuildProfile* EditorProject::GetActiveBuildProfile() const
{
    return FindBuildProfile(m_ActiveBuildProfileName);
}

FEditorBuildProfile* EditorProject::GetActiveBuildProfile()
{
    return FindBuildProfile(m_ActiveBuildProfileName);
}

void EditorProject::EnsureBuildProfiles()
{
    if (m_BuildProfiles.empty()) {
        m_BuildProfiles = BuildDefaultBuildProfiles();
    }

    for (FEditorBuildProfile& profile : m_BuildProfiles) {
        if (profile.Name.empty()) {
            profile.Name = GetTargetPlatformDisplayName(profile.TargetPlatform) + " " + profile.Configuration;
        }
        if (profile.Configuration.empty()) {
            profile.Configuration = "Debug";
        }
        if (profile.BuildDirectory.empty()) {
            profile.BuildDirectory = BuildDefaultBuildDirectoryRelativePath(
                profile.TargetPlatform,
                profile.Configuration);
        }
    }

    if (FindBuildProfile(m_ActiveBuildProfileName) == nullptr) {
        const FEditorBuildProfile* preferredWindowsDebug = FindBuildProfile("Windows Debug");
        if (preferredWindowsDebug != nullptr) {
            m_ActiveBuildProfileName = preferredWindowsDebug->Name;
        } else if (!m_BuildProfiles.empty()) {
            m_ActiveBuildProfileName = m_BuildProfiles.front().Name;
        } else {
            m_ActiveBuildProfileName.clear();
        }
    }
}

} // namespace ImWidgetV4Editor
