#pragma once

#include "../serialization/DocumentFormat.h"
#include "../toolchains/PlatformConfiguration.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

enum class EEditorLibraryIntegrationMode {
    Source,
    SDK
};

struct FEditorApplicationSettings {
    std::string Title;
    std::filesystem::path IconPath;
    int InitialWidth = 1280;
    int InitialHeight = 720;
    bool bEnableIniSettings = false;
    std::filesystem::path IniSettingsPath;
    bool bUseCustomHostChrome = false;
    bool bUseTitleBar = false;
    std::filesystem::path TitleBarDocumentRelativePath;
    bool bShowSystemButtons = true;
    bool bUseTitleBarMenus = false;
    EEditorLibraryIntegrationMode LibraryIntegrationMode = EEditorLibraryIntegrationMode::Source;
    std::filesystem::path SdkPackagePath;
    std::string MinimumSdkVersion;
    std::string DefaultTheme;
    std::string DefaultCulture;
    std::vector<std::filesystem::path> StringTablePaths;
    bool bGenerateInitializeStub = false;
    bool bGenerateTickStub = false;
};

class EditorProject {
public:
    static constexpr int FormatVersion = 6;

    static std::string GetManifestFileName();
    static std::filesystem::path BuildManifestFilePath(const std::filesystem::path& projectRoot);

    void Reset();

    bool CreateNew(
        const std::filesystem::path& projectRoot,
        const std::string& projectName,
        const std::string& namespaceName,
        const std::filesystem::path& startupDocumentRelativePath,
        const std::string& templateName = "Blank App");
    bool Load(const std::filesystem::path& manifestFilePath, std::string* outError = nullptr);
    bool Save(std::string* outError = nullptr) const;

    json ToJson() const;
    bool FromJson(
        const json& projectJson,
        const std::filesystem::path& manifestFilePath,
        std::string* outError = nullptr);

    bool IsValid() const;

    const std::string& GetProjectName() const { return m_ProjectName; }
    void SetProjectName(const std::string& projectName) { m_ProjectName = projectName; }

    const std::string& GetNamespaceName() const { return m_NamespaceName; }
    void SetNamespaceName(const std::string& namespaceName) { m_NamespaceName = namespaceName; }

    const std::string& GetTemplateName() const { return m_TemplateName; }
    void SetTemplateName(const std::string& templateName) { m_TemplateName = templateName; }

    const std::filesystem::path& GetProjectRoot() const { return m_ProjectRoot; }
    void SetProjectRoot(const std::filesystem::path& projectRoot) { m_ProjectRoot = projectRoot; }

    const std::filesystem::path& GetStartupDocumentRelativePath() const { return m_StartupDocumentRelativePath; }
    void SetStartupDocumentRelativePath(const std::filesystem::path& startupDocumentRelativePath)
    {
        m_StartupDocumentRelativePath = startupDocumentRelativePath;
    }

    const std::filesystem::path& GetTitleBarDocumentRelativePath() const
    {
        return m_ApplicationSettings.TitleBarDocumentRelativePath;
    }
    void SetTitleBarDocumentRelativePath(const std::filesystem::path& titleBarDocumentRelativePath)
    {
        m_ApplicationSettings.TitleBarDocumentRelativePath = titleBarDocumentRelativePath;
    }

    const FEditorApplicationSettings& GetApplicationSettings() const { return m_ApplicationSettings; }
    FEditorApplicationSettings& GetApplicationSettings() { return m_ApplicationSettings; }
    void SetApplicationSettings(const FEditorApplicationSettings& settings) { m_ApplicationSettings = settings; }

    const std::vector<FEditorBuildProfile>& GetBuildProfiles() const { return m_BuildProfiles; }
    std::vector<FEditorBuildProfile>& GetBuildProfiles() { return m_BuildProfiles; }

    const std::string& GetActiveBuildProfileName() const { return m_ActiveBuildProfileName; }
    bool SetActiveBuildProfileName(const std::string& profileName);
    const FEditorBuildProfile* FindBuildProfile(const std::string& profileName) const;
    FEditorBuildProfile* FindBuildProfile(const std::string& profileName);
    const FEditorBuildProfile* GetActiveBuildProfile() const;
    FEditorBuildProfile* GetActiveBuildProfile();

    std::filesystem::path GetManifestFilePath() const;
    std::filesystem::path GetStartupDocumentPath() const;
    std::filesystem::path GetTitleBarDocumentPath() const;

private:
    void EnsureBuildProfiles();

    std::string m_ProjectName;
    std::string m_NamespaceName;
    std::string m_TemplateName = "Blank App";
    std::filesystem::path m_ProjectRoot;
    std::filesystem::path m_StartupDocumentRelativePath;
    FEditorApplicationSettings m_ApplicationSettings;
    std::vector<FEditorBuildProfile> m_BuildProfiles;
    std::string m_ActiveBuildProfileName;
};

} // namespace ImWidgetV4Editor
