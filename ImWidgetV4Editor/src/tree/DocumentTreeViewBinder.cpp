#include "DocumentTreeViewBinder.h"
#include "../editor/EditorDocument.h"
#include "WidgetTreeDragDrop.h"

#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImTextBlock> MakeTreeDragPreview(const std::string& text)
{
    auto preview = std::make_shared<ImTextBlock>();
    preview->SetText(text);
    preview->SetFontSize(14.0f);
    preview->SetTextColor(FColor::White);
    preview->SetHitTestVisible(false);
    return preview;
}

bool IsDropCandidateContainer(const std::shared_ptr<ImWidget>& widget)
{
    return std::dynamic_pointer_cast<ImCanvasPanel>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImVerticalBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImHorizontalBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImScrollBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImTabView>(widget) != nullptr;
}

bool IsLogicalAncestorOf(
    const std::shared_ptr<EditorDocument>& document,
    const std::shared_ptr<ImWidget>& possibleAncestor,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!document || !possibleAncestor || !widget) {
        return false;
    }

    std::shared_ptr<ImWidget> current = widget;
    while (current) {
        if (current == possibleAncestor) {
            return true;
        }
        current = document->FindLogicalParent(current);
    }

    return false;
}

std::vector<std::shared_ptr<ImWidget>> GetLogicalChildren(const std::shared_ptr<ImWidget>& widget)
{
    std::vector<std::shared_ptr<ImWidget>> children;
    if (!widget) {
        return children;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        children.reserve(static_cast<std::size_t>(tabView->GetTabCount()));
        for (int index = 0; index < tabView->GetTabCount(); ++index) {
            const FTabViewItem* tab = tabView->GetTab(index);
            if (tab && tab->Content) {
                children.push_back(tab->Content);
            }
        }
        return children;
    }

    return widget->GetChildren();
}

} // namespace

void DocumentTreeViewBinder::Bind(
    const std::shared_ptr<ImTextOutlineView>& treeView,
    const std::shared_ptr<ImDesignerSurface>& designerSurface,
    const std::shared_ptr<EditorDocument>& document)
{
    m_TreeView = treeView;
    m_DesignerSurface = designerSurface;
    SetDocument(document);

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
    m_TreeView->OnItemDragDetected.Clear();
    m_TreeView->OnItemDragDetected.AddLambda(
        [this](ImTextOutlineView&, ImTextOutlineItem& item, std::shared_ptr<FDragDropOperation>& outOperation) {
            if (outOperation || !m_TreeView) {
                return;
            }

            auto widget = ResolveWidget(&item);
            auto document = m_Document.lock();
            if (!widget || !document) {
                return;
            }

            const std::string widgetId = document->GetWidgetId(widget);
            if (widgetId.empty()) {
                return;
            }

            auto payload = std::make_shared<WidgetTreeDragDropPayload>();
            payload->WidgetId = widgetId;
            payload->Label = BuildItemLabel(widget);

            auto operation = std::make_shared<FDragDropOperation>();
            operation->Payload = payload;
            operation->PreviewWidget = MakeTreeDragPreview(payload->Label);
            operation->PreviewOffset = FVector2(14.0f, 16.0f);
            outOperation = operation;
        });
    m_TreeView->OnItemDropTest.Clear();
    m_TreeView->OnItemDropTest.AddLambda(
        [this](ImTextOutlineView&, ImTextOutlineItem& item, ETextOutlineDropZone zone, const std::shared_ptr<FDragDropOperation>& operation, FVector2, bool& bAccepted) {
            auto payload = std::dynamic_pointer_cast<WidgetTreeDragDropPayload>(operation ? operation->Payload : nullptr);
            auto document = m_Document.lock();
            auto targetWidget = ResolveWidget(&item);
            if (!payload || !document || !targetWidget) {
                bAccepted = false;
                return;
            }

            auto sourceWidget = document->FindWidgetById(payload->WidgetId);
            if (!sourceWidget ||
                sourceWidget == targetWidget ||
                IsLogicalAncestorOf(document, sourceWidget, targetWidget)) {
                bAccepted = false;
                return;
            }

            if (zone == ETextOutlineDropZone::OnItem && !IsDropCandidateContainer(targetWidget)) {
                bAccepted = false;
                return;
            }

            bAccepted = true;
        });
}

void DocumentTreeViewBinder::SetDocument(const std::shared_ptr<EditorDocument>& document)
{
    m_Document = document;
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

    const auto children = GetLogicalChildren(parentWidget);
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
