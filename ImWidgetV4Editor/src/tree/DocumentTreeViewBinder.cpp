#include "DocumentTreeViewBinder.h"
#include "../editor/EditorDocument.h"
#include "../editor/LogicalWidgetTree.h"
#include "../editor/WidgetTypeIcon.h"
#include "../palette/WidgetPaletteDragDrop.h"
#include "WidgetTreeDragDrop.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImWidget> MakeTreeDragPreview(const FImageBrush& iconBrush, const std::string& text)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);
    row->SetHitTestVisible(false);

    if (iconBrush.IsValid()) {
        auto image = std::make_shared<ImImage>();
        image->SetBrush(iconBrush);
        image->SetDesiredSize(FVector2(16.0f, 16.0f));
        image->SetHitTestVisible(false);
        row->AddChild(image);
    }

    auto preview = std::make_shared<ImTextBlock>();
    preview->SetText(text);
    preview->SetFontSize(14.0f);
    preview->SetTextColor(FColor::White);
    preview->SetHitTestVisible(false);
    row->AddChild(preview);
    return row;
}

bool IsDropCandidateContainer(const std::shared_ptr<ImWidget>& widget)
{
    return std::dynamic_pointer_cast<ImCanvasPanel>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImButton>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImExpandableBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImVerticalBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImHorizontalBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImScrollBox>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImTabView>(widget) != nullptr;
}

bool CanAcceptAsSingleContent(const std::shared_ptr<ImWidget>& widget)
{
    if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        return button->GetContent() == nullptr;
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        return expandableBox->GetHeader() == nullptr || expandableBox->GetBody() == nullptr;
    }

    return true;
}

bool CanInsertIntoOrderedSiblingParent(const std::shared_ptr<ImWidget>& parent)
{
    return std::dynamic_pointer_cast<ImVerticalBox>(parent) != nullptr ||
        std::dynamic_pointer_cast<ImHorizontalBox>(parent) != nullptr ||
        std::dynamic_pointer_cast<ImScrollBox>(parent) != nullptr ||
        std::dynamic_pointer_cast<ImTabView>(parent) != nullptr;
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
            FImageBrush iconBrush;
            if (m_TreeView->GetApplication() != nullptr) {
                if (const auto icon = TryGetWidgetTypeIcon(widget->GetTypeName())) {
                    iconBrush = m_TreeView->GetApplication()->GetCoreIconBrush(*icon, FColor::FromBytes(228, 233, 241));
                }
            }
            operation->PreviewWidget = MakeTreeDragPreview(iconBrush, payload->Label);
            operation->PreviewOffset = FVector2(14.0f, 16.0f);
            outOperation = operation;
        });
    m_TreeView->OnItemDropTest.Clear();
    m_TreeView->OnItemDropTest.AddLambda(
        [this](ImTextOutlineView&, ImTextOutlineItem& item, ETextOutlineDropZone zone, const std::shared_ptr<FDragDropOperation>& operation, FVector2, bool& bAccepted) {
            auto document = m_Document.lock();
            auto targetWidget = ResolveWidget(&item);
            if (!operation || !operation->Payload || !document || !targetWidget) {
                bAccepted = false;
                return;
            }

            if (auto treePayload = std::dynamic_pointer_cast<WidgetTreeDragDropPayload>(operation->Payload)) {
                auto sourceWidget = document->FindWidgetById(treePayload->WidgetId);
                if (!sourceWidget ||
                    sourceWidget == targetWidget ||
                    IsLogicalAncestorOf(document, sourceWidget, targetWidget)) {
                    bAccepted = false;
                    return;
                }

                if (zone == ETextOutlineDropZone::OnItem) {
                    bAccepted = IsDropCandidateContainer(targetWidget) && CanAcceptAsSingleContent(targetWidget);
                    return;
                }

                auto targetParent = document->FindLogicalParent(targetWidget);
                bAccepted = CanInsertIntoOrderedSiblingParent(targetParent);
                return;
            }

            if (std::dynamic_pointer_cast<WidgetPalettePayload>(operation->Payload)) {
                if (zone == ETextOutlineDropZone::OnItem) {
                    bAccepted = IsDropCandidateContainer(targetWidget) && CanAcceptAsSingleContent(targetWidget);
                    return;
                }

                auto targetParent = document->FindLogicalParent(targetWidget);
                bAccepted = CanInsertIntoOrderedSiblingParent(targetParent);
                return;
            }

            bAccepted = false;
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

    if (const auto icon = TryGetWidgetTypeIcon(rootWidget->GetTypeName())) {
        rootItem->IconType = static_cast<int>(*icon);
    }
    if (ImApplication* application = m_TreeView->GetApplication()) {
        if (const auto icon = TryGetWidgetTypeIcon(rootWidget->GetTypeName())) {
            rootItem->IconBrush = application->GetCoreIconBrush(*icon, FColor::FromBytes(214, 222, 234));
        }
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

    const std::size_t childCount = LogicalWidgetTree::GetLogicalChildCount(parentWidget);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        const auto child = LogicalWidgetTree::GetLogicalChildAt(parentWidget, childIndex);
        if (!child) {
            continue;
        }

        std::string childLabel = BuildItemLabel(child);
        if (const char* roleName = LogicalWidgetTree::GetLogicalChildRoleName(parentWidget, childIndex)) {
            childLabel = std::string(roleName) + ": " + childLabel;
        }

        ImTextOutlineItem* childItem = m_TreeView->AddChildItem(parentItem, childLabel);
        if (!childItem) {
            continue;
        }

        if (const auto icon = TryGetWidgetTypeIcon(child->GetTypeName())) {
            childItem->IconType = static_cast<int>(*icon);
        }
        if (ImApplication* application = m_TreeView->GetApplication()) {
            if (const auto icon = TryGetWidgetTypeIcon(child->GetTypeName())) {
                childItem->IconBrush = application->GetCoreIconBrush(*icon, FColor::FromBytes(214, 222, 234));
            }
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
