#pragma once

#include "../serialization/DocumentFormat.h"

#include <imwidgetv4/core/Widget.h>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

namespace ImWidgetV4Editor {

class EditorDocument {
public:
    EditorDocument();

    void NewDocument(const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget, const std::string& title = "Untitled");

    bool Load(const std::filesystem::path& filePath, std::string* outError = nullptr);
    bool Save(std::string* outError = nullptr);
    bool SaveAs(const std::filesystem::path& filePath, std::string* outError = nullptr);
    json ExportDocumentJson() const;
    bool ImportDocumentJson(const json& documentJson, std::string* outError = nullptr);

    void SetRootWidget(const std::shared_ptr<ImWidgetV4::ImWidget>& rootWidget);
    std::shared_ptr<ImWidgetV4::ImWidget> GetRootWidget() const { return m_RootWidget; }
    std::string GetWidgetId(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    std::shared_ptr<ImWidgetV4::ImWidget> FindWidgetById(const std::string& widgetId);
    std::shared_ptr<ImWidgetV4::ImWidget> FindLogicalParent(const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;

    void SetDirty(bool bDirty) { m_bDirty = bDirty; }
    bool IsDirty() const { return m_bDirty; }

    bool HasFilePath() const { return !m_FilePath.empty(); }
    const std::filesystem::path& GetFilePath() const { return m_FilePath; }

    void SetDisplayTitle(const std::string& title) { m_DisplayTitle = title; }
    const std::string& GetDisplayTitle() const { return m_DisplayTitle; }
    std::string GetTabTitle() const;

private:
    json BuildDocumentJson() const;
    bool LoadFromDocumentJson(const json& documentJson, std::string* outError);
    void RebuildWidgetIdIndex(bool bAssignMissingIds);
    void RebuildWidgetIdIndexRecursive(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        bool bAssignMissingIds,
        const std::unordered_map<const ImWidgetV4::ImWidget*, std::string>* previousIds);
    void ApplyWidgetIdsFromJson(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const json& widgetJson);
    std::string GenerateNextWidgetId();
    void TrackExistingWidgetId(const std::string& widgetId);

    std::filesystem::path m_FilePath;
    std::string m_DisplayTitle;
    bool m_bDirty = false;
    std::shared_ptr<ImWidgetV4::ImWidget> m_RootWidget;
    std::unordered_map<const ImWidgetV4::ImWidget*, std::string> m_WidgetIds;
    std::unordered_map<std::string, std::weak_ptr<ImWidgetV4::ImWidget>> m_WidgetsById;
    int m_NextWidgetId = 1;
};

} // namespace ImWidgetV4Editor
