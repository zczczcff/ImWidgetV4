#include "DocumentTreeViewBinder.h"

#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/TextOutlineView.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

void DocumentTreeViewBinder::Bind(
    const std::shared_ptr<ImTextOutlineView>& treeView,
    const std::shared_ptr<ImDesignerSurface>& designerSurface)
{
    m_TreeView = treeView;
    m_DesignerSurface = designerSurface;

    if (!m_TreeView) {
        return;
    }

    m_TreeView->OnSelectionChanged.Clear();
    m_TreeView->OnSelectionChanged.AddLambda(
        [this](ImTextOutlineView&, ImTextOutlineItem* selectedItem) {
            if (m_bSyncingSelection || !m_DesignerSurface) {
                return;
            }

            auto it = m_ItemToWidget.find(selectedItem);
            m_bSyncingSelection = true;
            if (it != m_ItemToWidget.end()) {
                m_DesignerSurface->SetSelectedWidget(it->second);
            } else {
                m_DesignerSurface->ClearSelection();
            }
            m_bSyncingSelection = false;
        });
}

void DocumentTreeViewBinder::RebuildFromRoot(
    const std::shared_ptr<ImWidget>& rootWidget,
    const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (!m_TreeView) {
        return;
    }

    m_ItemToWidget.clear();
    m_WidgetToItem.clear();
    m_TreeView->ClearItems();

    if (!rootWidget) {
        return;
    }

    ImTextOutlineItem* rootItem = m_TreeView->AddRootItem(BuildItemLabel(rootWidget));
    if (!rootItem) {
        return;
    }

    rootItem->Expanded = true;
    m_ItemToWidget[rootItem] = rootWidget;
    m_WidgetToItem[rootWidget.get()] = rootItem;
    RebuildChildren(rootItem, rootWidget);
    SyncSelectionFromDesigner(selectedWidget);
}

void DocumentTreeViewBinder::SyncSelectionFromDesigner(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (!m_TreeView) {
        return;
    }

    m_bSyncingSelection = true;
    if (!selectedWidget) {
        m_TreeView->ClearSelection();
    } else {
        auto it = m_WidgetToItem.find(selectedWidget.get());
        if (it != m_WidgetToItem.end()) {
            m_TreeView->SetSelectedItem(it->second);
            m_TreeView->ScrollToItem(it->second);
        } else {
            m_TreeView->ClearSelection();
        }
    }
    m_bSyncingSelection = false;
}

std::shared_ptr<ImWidget> DocumentTreeViewBinder::ResolveWidget(ImTextOutlineItem* item) const
{
    auto it = m_ItemToWidget.find(item);
    if (it == m_ItemToWidget.end()) {
        return nullptr;
    }

    return it->second;
}

ImTextOutlineItem* DocumentTreeViewBinder::ResolveItem(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget) {
        return nullptr;
    }

    auto it = m_WidgetToItem.find(widget.get());
    if (it == m_WidgetToItem.end()) {
        return nullptr;
    }

    return it->second;
}

void DocumentTreeViewBinder::RebuildChildren(
    ImTextOutlineItem* parentItem,
    const std::shared_ptr<ImWidget>& parentWidget)
{
    if (!parentItem || !parentWidget || !m_TreeView) {
        return;
    }

    const auto& children = parentWidget->GetChildren();
    for (const auto& child : children) {
        if (!child) {
            continue;
        }

        ImTextOutlineItem* childItem = m_TreeView->AddChildItem(parentItem, BuildItemLabel(child));
        if (!childItem) {
            continue;
        }

        childItem->Expanded = true;
        m_ItemToWidget[childItem] = child;
        m_WidgetToItem[child.get()] = childItem;
        RebuildChildren(childItem, child);
    }
}

std::string DocumentTreeViewBinder::BuildItemLabel(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget) {
        return "<null>";
    }

    std::string label = widget->GetTypeName();
    if (!widget->GetName().empty()) {
        label += " [" + widget->GetName() + "]";
    }
    return label;
}

} // namespace ImWidgetV4Editor
