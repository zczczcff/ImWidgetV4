#pragma once

#include <memory>
#include <string>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorDocument;

class SelectionModel {
public:
    void Clear();

    void SetSelectedWidget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<EditorDocument>& document);

    void SetSelectedWidgetId(const std::string& widgetId);
    const std::string& GetSelectedWidgetId() const { return m_SelectedWidgetId; }

    std::shared_ptr<ImWidgetV4::ImWidget> ResolveSelectedWidget(
        const std::shared_ptr<EditorDocument>& document) const;

    bool HasSelection() const { return !m_SelectedWidgetId.empty(); }

private:
    std::string m_SelectedWidgetId;
};

} // namespace ImWidgetV4Editor
