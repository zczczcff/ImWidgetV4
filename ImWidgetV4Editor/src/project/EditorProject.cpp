#include "EditorProject.h"

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
    m_ProjectRoot.clear();
    m_StartupDocumentRelativePath.clear();
}

bool EditorProject::CreateNew(
    const std::filesystem::path& projectRoot,
    const std::string& projectName,
    const std::string& namespaceName,
    const std::filesystem::path& startupDocumentRelativePath)
{
    Reset();
    m_ProjectRoot = projectRoot.lexically_normal();
    m_ProjectName = projectName;
    m_NamespaceName = namespaceName;
    m_StartupDocumentRelativePath = NormalizeStoredRelativePath(startupDocumentRelativePath);
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
    projectSection["StartupDocument"] = m_StartupDocumentRelativePath.generic_string();
    projectJson["Project"] = std::move(projectSection);
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

    if (projectJson.value("Version", 0) != FormatVersion) {
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
    m_StartupDocumentRelativePath = NormalizeStoredRelativePath(
        std::filesystem::path(projectSection.value("StartupDocument", std::string())));

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
           !m_ProjectRoot.empty() &&
           !m_StartupDocumentRelativePath.empty() &&
           !m_StartupDocumentRelativePath.is_absolute();
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

} // namespace ImWidgetV4Editor
