#include "EditorProject.h"

#include <algorithm>
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

FEditorApplicationSettings BuildDefaultApplicationSettings(const std::string& projectName = std::string())
{
    FEditorApplicationSettings settings;
    settings.Title = projectName;
    settings.InitialWidth = 1280;
    settings.InitialHeight = 720;
    settings.bEnableIniSettings = false;
    settings.IniSettingsPath.clear();
    settings.bUseCustomHostChrome = false;
    settings.bUseTitleBar = false;
    settings.bShowSystemButtons = true;
    settings.DefaultTheme.clear();
    settings.DefaultCulture.clear();
    return settings;
}

json ApplicationSettingsToJson(const FEditorApplicationSettings& settings)
{
    json settingsJson;
    settingsJson["Title"] = settings.Title;
    settingsJson["InitialWidth"] = settings.InitialWidth;
    settingsJson["InitialHeight"] = settings.InitialHeight;
    settingsJson["EnableIniSettings"] = settings.bEnableIniSettings;
    settingsJson["IniSettingsPath"] = NormalizeStoredRelativePath(settings.IniSettingsPath).generic_string();
    settingsJson["UseCustomHostChrome"] = settings.bUseCustomHostChrome;
    settingsJson["UseTitleBar"] = settings.bUseTitleBar;
    settingsJson["ShowSystemButtons"] = settings.bShowSystemButtons;
    settingsJson["DefaultTheme"] = settings.DefaultTheme;
    settingsJson["DefaultCulture"] = settings.DefaultCulture;
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
    settings.bShowSystemButtons = settingsJson.value("ShowSystemButtons", settings.bShowSystemButtons);
    settings.DefaultTheme = settingsJson.value("DefaultTheme", settings.DefaultTheme);
    settings.DefaultCulture = settingsJson.value("DefaultCulture", settings.DefaultCulture);
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
    if (version != 1 && version != 2 && version != FormatVersion) {
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
