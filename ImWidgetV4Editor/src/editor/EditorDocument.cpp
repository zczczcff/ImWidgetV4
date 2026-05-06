#include "EditorDocument.h"

#include "../serialization/WidgetSerializer.h"

#include <imwidgetv4/widgets/TabView.h>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace ImWidgetV4Editor {

namespace {

constexpr const char* kEditorIdFieldName = "EditorId";

int ParseWidgetIdSuffix(const std::string& widgetId)
{
    if (widgetId.size() <= 1 || widgetId.front() != 'w') {
        return -1;
    }

    try {
        return std::stoi(widgetId.substr(1));
    } catch (...) {
        return -1;
    }
}

std::size_t GetLogicalChildCount(const std::shared_ptr<ImWidgetV4::ImWidget>& widget)
{
    if (!widget) {
        return 0;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImWidgetV4::ImTabView>(widget)) {
        return static_cast<std::size_t>(tabView->GetTabCount());
    }

    return widget->GetChildren().size();
}

std::shared_ptr<ImWidgetV4::ImWidget> GetLogicalChildAt(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    std::size_t childIndex)
{
    if (!widget) {
        return nullptr;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImWidgetV4::ImTabView>(widget)) {
        const ImWidgetV4::FTabViewItem* tab = tabView->GetTab(static_cast<int>(childIndex));
        return tab ? tab->Content : nullptr;
    }

    const auto& children = widget->GetChildren();
    if (childIndex >= children.size()) {
        return nullptr;
    }

    return children[childIndex];
}

json* ResolveLogicalChildJson(json& widgetJson, const std::shared_ptr<ImWidgetV4::ImWidget>& widget, std::size_t childIndex)
{
    if (!widgetJson.is_object()) {
        return nullptr;
    }

    if (std::dynamic_pointer_cast<ImWidgetV4::ImTabView>(widget)) {
        if (!widgetJson.contains("TabItems") || !widgetJson["TabItems"].is_array()) {
            return nullptr;
        }

        json& tabItems = widgetJson["TabItems"];
        if (childIndex >= tabItems.size()) {
            return nullptr;
        }

        json& tabItem = tabItems[childIndex];
        if (!tabItem.is_object()) {
            return nullptr;
        }

        if (!tabItem.contains("Content")) {
            tabItem["Content"] = json();
        }
        return &tabItem["Content"];
    }

    if (!widgetJson.contains("Children") || !widgetJson["Children"].is_array()) {
        return nullptr;
    }

    json& children = widgetJson["Children"];
    if (childIndex >= children.size()) {
        return nullptr;
    }

    return &children[childIndex];
}

} // namespace

EditorDocument::EditorDocument() = default;

void EditorDocument::NewDocument(const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget, const std::string& title)
{
    m_FilePath.clear();
    m_DisplayTitle = title;
    m_RootWidget = rootWidget;
    m_bDirty = false;
    m_NextWidgetId = 1;
    RebuildWidgetIdIndex(true);
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
    RebuildWidgetIdIndex(true);
}

json EditorDocument::ExportDocumentJson() const
{
    return const_cast<EditorDocument*>(this)->BuildDocumentJson();
}

bool EditorDocument::ImportDocumentJson(const json& documentJson, std::string* outError)
{
    return LoadFromDocumentJson(documentJson, outError);
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

std::string EditorDocument::GetWidgetId(const std::shared_ptr<ImWidgetV4::ImWidget>& widget)
{
    if (!widget) {
        return std::string();
    }

    auto it = m_WidgetIds.find(widget.get());
    if (it != m_WidgetIds.end()) {
        return it->second;
    }

    RebuildWidgetIdIndex(true);
    it = m_WidgetIds.find(widget.get());
    return it != m_WidgetIds.end() ? it->second : std::string();
}

std::shared_ptr<ImWidgetV4::ImWidget> EditorDocument::FindWidgetById(const std::string& widgetId)
{
    if (widgetId.empty()) {
        return nullptr;
    }

    auto it = m_WidgetsById.find(widgetId);
    if (it != m_WidgetsById.end()) {
        if (auto widget = it->second.lock()) {
            return widget;
        }
    }

    RebuildWidgetIdIndex(true);
    it = m_WidgetsById.find(widgetId);
    return it != m_WidgetsById.end() ? it->second.lock() : nullptr;
}

json EditorDocument::BuildDocumentJson() const
{
    EditorDocument* self = const_cast<EditorDocument*>(this);
    self->RebuildWidgetIdIndex(true);

    json documentJson;
    documentJson["Format"] = "ImWidgetV4EditorDocument";
    documentJson["Version"] = kEditorDocumentFormatVersion;
    documentJson["Title"] = m_DisplayTitle;
    documentJson["RootWidget"] = WidgetSerializer::SerializeWidgetTree(m_RootWidget);

    std::function<void(const std::shared_ptr<ImWidgetV4::ImWidget>&, json&)> annotateIds =
        [self, &annotateIds](const std::shared_ptr<ImWidgetV4::ImWidget>& widget, json& widgetJson) {
            if (!widget || widgetJson.is_null() || !widgetJson.is_object()) {
                return;
            }

            const auto idIt = self->m_WidgetIds.find(widget.get());
            if (idIt != self->m_WidgetIds.end()) {
                widgetJson[kEditorIdFieldName] = idIt->second;
            }

            const std::size_t childCount = GetLogicalChildCount(widget);
            for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
                json* childJson = ResolveLogicalChildJson(widgetJson, widget, childIndex);
                if (childJson == nullptr) {
                    continue;
                }

                annotateIds(GetLogicalChildAt(widget, childIndex), *childJson);
            }
        };
    annotateIds(m_RootWidget, documentJson["RootWidget"]);
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
    if (!widgetResult.bSuccess) {
        if (outError) {
            *outError = widgetResult.ErrorMessage.empty()
                ? "Failed to deserialize root widget."
                : widgetResult.ErrorMessage;
        }
        return false;
    }

    m_DisplayTitle = documentJson.value("Title", "");
    m_RootWidget = widgetResult.Widget;
    m_NextWidgetId = 1;
    RebuildWidgetIdIndex(false);
    ApplyWidgetIdsFromJson(m_RootWidget, documentJson.at("RootWidget"));
    RebuildWidgetIdIndex(true);
    return true;
}

void EditorDocument::RebuildWidgetIdIndex(bool bAssignMissingIds)
{
    const auto previousIds = m_WidgetIds;
    m_WidgetIds.clear();
    m_WidgetsById.clear();
    if (bAssignMissingIds && m_NextWidgetId <= 0) {
        m_NextWidgetId = 1;
    }

    RebuildWidgetIdIndexRecursive(m_RootWidget, bAssignMissingIds, &previousIds);
}

void EditorDocument::RebuildWidgetIdIndexRecursive(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    bool bAssignMissingIds,
    const std::unordered_map<const ImWidgetV4::ImWidget*, std::string>* previousIds)
{
    if (!widget) {
        return;
    }

    std::string widgetId;
    if (previousIds != nullptr) {
        auto previousIt = previousIds->find(widget.get());
        if (previousIt != previousIds->end()) {
            widgetId = previousIt->second;
        }
    }

    if (widgetId.empty() && bAssignMissingIds) {
        widgetId = GenerateNextWidgetId();
    }

    if (!widgetId.empty()) {
        m_WidgetIds[widget.get()] = widgetId;
        m_WidgetsById[widgetId] = widget;
        TrackExistingWidgetId(widgetId);
    }

    const std::size_t childCount = GetLogicalChildCount(widget);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        RebuildWidgetIdIndexRecursive(GetLogicalChildAt(widget, childIndex), bAssignMissingIds, previousIds);
    }
}

void EditorDocument::ApplyWidgetIdsFromJson(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    const json& widgetJson)
{
    if (!widget || widgetJson.is_null() || !widgetJson.is_object()) {
        return;
    }

    std::string widgetId = widgetJson.value(kEditorIdFieldName, std::string());
    if (widgetId.empty()) {
        widgetId = GenerateNextWidgetId();
    }

    m_WidgetIds[widget.get()] = widgetId;
    m_WidgetsById[widgetId] = widget;
    TrackExistingWidgetId(widgetId);

    const std::size_t childCount = GetLogicalChildCount(widget);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        std::shared_ptr<ImWidgetV4::ImWidget> childWidget = GetLogicalChildAt(widget, childIndex);
        if (!childWidget) {
            continue;
        }

        const json* childJson = nullptr;
        if (std::dynamic_pointer_cast<ImWidgetV4::ImTabView>(widget)) {
            if (widgetJson.contains("TabItems") && widgetJson["TabItems"].is_array() && childIndex < widgetJson["TabItems"].size()) {
                const json& tabItem = widgetJson["TabItems"][childIndex];
                if (tabItem.is_object() && tabItem.contains("Content")) {
                    childJson = &tabItem["Content"];
                }
            }
        } else if (widgetJson.contains("Children") && widgetJson["Children"].is_array() && childIndex < widgetJson["Children"].size()) {
            childJson = &widgetJson["Children"][childIndex];
        }

        if (childJson) {
            ApplyWidgetIdsFromJson(childWidget, *childJson);
        } else {
            RebuildWidgetIdIndexRecursive(childWidget, true, nullptr);
        }
    }
}

std::string EditorDocument::GenerateNextWidgetId()
{
    while (true) {
        const std::string candidate = "w" + std::to_string(m_NextWidgetId++);
        if (m_WidgetsById.find(candidate) == m_WidgetsById.end()) {
            return candidate;
        }
    }
}

void EditorDocument::TrackExistingWidgetId(const std::string& widgetId)
{
    const int suffix = ParseWidgetIdSuffix(widgetId);
    if (suffix >= 0) {
        m_NextWidgetId = std::max(m_NextWidgetId, suffix + 1);
    }
}

} // namespace ImWidgetV4Editor
