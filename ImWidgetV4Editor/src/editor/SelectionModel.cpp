#include "SelectionModel.h"

#include "EditorDocument.h"

namespace ImWidgetV4Editor {

void SelectionModel::Clear()
{
    m_SelectedWidgetId.clear();
}

void SelectionModel::SetSelectedWidget(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    const std::shared_ptr<EditorDocument>& document)
{
    if (!widget || !document) {
        m_SelectedWidgetId.clear();
        return;
    }

    m_SelectedWidgetId = document->GetWidgetId(widget);
}

void SelectionModel::SetSelectedWidgetId(const std::string& widgetId)
{
    m_SelectedWidgetId = widgetId;
}

std::shared_ptr<ImWidgetV4::ImWidget> SelectionModel::ResolveSelectedWidget(
    const std::shared_ptr<EditorDocument>& document) const
{
    if (!document || m_SelectedWidgetId.empty()) {
        return nullptr;
    }

    return document->FindWidgetById(m_SelectedWidgetId);
}

} // namespace ImWidgetV4Editor
