#pragma once

#include "../serialization/DocumentFormat.h"

#include <filesystem>
#include <string>

namespace ImWidgetV4Editor {

class EditorProject {
public:
    static constexpr int FormatVersion = 2;

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

    std::filesystem::path GetManifestFilePath() const;
    std::filesystem::path GetStartupDocumentPath() const;

private:
    std::string m_ProjectName;
    std::string m_NamespaceName;
    std::string m_TemplateName = "Blank App";
    std::filesystem::path m_ProjectRoot;
    std::filesystem::path m_StartupDocumentRelativePath;
};

} // namespace ImWidgetV4Editor
