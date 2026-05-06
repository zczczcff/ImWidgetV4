#include "EditorDocument.h"

#include "../serialization/WidgetSerializer.h"

#include <fstream>
#include <sstream>

namespace ImWidgetV4Editor {

EditorDocument::EditorDocument() = default;

void EditorDocument::NewDocument(const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget, const std::string& title)
{
    m_FilePath.clear();
    m_DisplayTitle = title;
    m_RootWidget = rootWidget;
    m_bDirty = false;
}

bool EditorDocument::Load(const std::filesystem::path& filePath, std::string* outError)
{
    try {
        std::ifstream stream(filePath);
        if (!stream.is_open()) {
            if (outError) {
                *outError = "Failed to open file for reading: " + filePath.string();
            }
            return false;
        }

        json documentJson;
        stream >> documentJson;
        if (!LoadFromDocumentJson(documentJson, outError)) {
            return false;
        }

        m_FilePath = filePath;
        m_bDirty = false;
        return true;
    } catch (const std::exception& e) {
        if (outError) {
            *outError = e.what();
        }
        return false;
    }
}

bool EditorDocument::Save(std::string* outError)
{
    if (m_FilePath.empty()) {
        if (outError) {
            *outError = "Document has no file path. Use SaveAs first.";
        }
        return false;
    }

    return SaveAs(m_FilePath, outError);
}

bool EditorDocument::SaveAs(const std::filesystem::path& filePath, std::string* outError)
{
    try {
        json documentJson = BuildDocumentJson();

        std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            if (outError) {
                *outError = "Failed to open file for writing: " + filePath.string();
            }
            return false;
        }

        stream << documentJson.dump(2);
        stream.flush();

        if (!stream.good()) {
            if (outError) {
                *outError = "Failed to write document JSON.";
            }
            return false;
        }

        m_FilePath = filePath;
        if (m_DisplayTitle.empty()) {
            m_DisplayTitle = filePath.stem().string();
        }
        m_bDirty = false;
        return true;
    } catch (const std::exception& e) {
        if (outError) {
            *outError = e.what();
        }
        return false;
    }
}

void EditorDocument::SetRootWidget(const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget)
{
    m_RootWidget = rootWidget;
    m_bDirty = true;
}

std::string EditorDocument::GetTabTitle() const
{
    std::string title = m_DisplayTitle;
    if (title.empty()) {
        title = m_FilePath.empty() ? "Untitled" : m_FilePath.stem().string();
    }

    if (m_bDirty) {
        title += "*";
    }

    return title;
}

json EditorDocument::BuildDocumentJson() const
{
    json documentJson;
    documentJson["Format"] = "ImWidgetV4EditorDocument";
    documentJson["Version"] = kEditorDocumentFormatVersion;
    documentJson["Title"] = m_DisplayTitle;
    documentJson["RootWidget"] = WidgetSerializer::SerializeWidgetTree(m_RootWidget);
    return documentJson;
}

bool EditorDocument::LoadFromDocumentJson(const json& documentJson, std::string* outError)
{
    if (!documentJson.is_object()) {
        if (outError) {
            *outError = "Document JSON must be an object.";
        }
        return false;
    }

    const std::string format = documentJson.value("Format", "");
    if (format != "ImWidgetV4EditorDocument") {
        if (outError) {
            *outError = "Unsupported document format.";
        }
        return false;
    }

    const int version = documentJson.value("Version", 0);
    if (version != kEditorDocumentFormatVersion) {
        if (outError) {
            *outError = "Unsupported document version.";
        }
        return false;
    }

    if (!documentJson.contains("RootWidget")) {
        if (outError) {
            *outError = "Document JSON is missing RootWidget.";
        }
        return false;
    }

    FWidgetSerializationResult widgetResult = WidgetSerializer::DeserializeWidgetTree(documentJson.at("RootWidget"));
    if (!widgetResult.bSuccess || !widgetResult.Widget) {
        if (outError) {
            *outError = widgetResult.ErrorMessage.empty()
                ? "Failed to deserialize root widget."
                : widgetResult.ErrorMessage;
        }
        return false;
    }

    m_DisplayTitle = documentJson.value("Title", "");
    m_RootWidget = widgetResult.Widget;
    return true;
}

} // namespace ImWidgetV4Editor
