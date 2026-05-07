#pragma once

#include "../commands/CommandStack.h"
#include "EditorDocument.h"

#include <imwidgetv4/core/Application.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
enum class EDesignerTransformHandle : std::uint8_t;
enum class ETextOutlineDropZone : std::uint8_t;
struct FDragDropOperation;
struct FPopupMenuItem;
class ImDesignerSurface;
class ImPopupMenu;
class ImScrollBox;
class ImTabView;
class ImTextBlock;
class ImTextOutlineItem;
class ImTextOutlineView;
class ImWidget;
class ImWindow;
}

namespace ImWidgetV4Editor {

class DocumentTreeViewBinder;
class ReflectionDetailsView;

class EditorSession : public std::enable_shared_from_this<EditorSession> {
public:
    explicit EditorSession(std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);

    void BindDocumentWidgets(
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        int documentTabIndex,
        const std::shared_ptr<ImWidgetV4::ImScrollBox>& documentHost,
        const std::shared_ptr<ImWidgetV4::ImScrollBox>& previewHost,
        const std::shared_ptr<ImWidgetV4::ImDesignerSurface>& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& widgetTreeView,
        const std::shared_ptr<ReflectionDetailsView>& detailsView,
        const std::shared_ptr<ImWidgetV4::ImTextBlock>& outputText);

    const std::shared_ptr<EditorDocument>& GetDocument() const { return m_Document; }
    std::string GetDocumentTabTitle() const;

    bool NewDocument();
    bool OpenDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocumentAs(ImWidgetV4::ImApplication& app);
    bool DeleteSelectedWidget();
    bool CutSelectedWidget();
    bool CopySelectedWidget();
    bool PasteCopiedWidget();
    bool DuplicateSelectedWidget();
    bool Undo();
    bool Redo();
    bool CanUndo() const;
    bool CanRedo() const;
    std::string GetUndoLabel() const;
    std::string GetRedoLabel() const;

    void LogStatus(const std::string& text);
    bool ApplyDocumentSnapshot(
        const json& documentJson,
        const std::string& selectionId,
        bool bDirty);

private:
    struct FDocumentSnapshot {
        json DocumentJson;
        std::string SelectionId;
        bool bDirty = false;
    };

    std::shared_ptr<EditorDocument> CreateDefaultDocument() const;
    void ApplyDocumentToUi();
    void HandleDesignerSelectionChanged(
        ImWidgetV4::ImDesignerSurface& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void HandleDesignerDrop(
        ImWidgetV4::ImDesignerSurface& designerSurface,
        const std::shared_ptr<ImWidgetV4::FDragDropOperation>& operation,
        const ImWidgetV4::FVector2& position,
        bool& bHandled);
    void HandleWidgetTreeContextMenuRequested(
        ImWidgetV4::ImTextOutlineView& treeView,
        ImWidgetV4::ImTextOutlineItem& item,
        ImWidgetV4::FVector2 position);
    void HandleWidgetTreeItemDropped(
        ImWidgetV4::ImTextOutlineView& treeView,
        ImWidgetV4::ImTextOutlineItem& item,
        ImWidgetV4::ETextOutlineDropZone zone,
        const std::shared_ptr<ImWidgetV4::FDragDropOperation>& operation,
        ImWidgetV4::FVector2 position,
        bool& bHandled);
    void UpdateSelectionDetails(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void HandlePropertyValueCommitted(
        ReflectionDetailsView& detailsView,
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const std::string& propertyClassName,
        const std::string& propertyName,
        const json& value);
    std::filesystem::path ResolveDialogDirectory() const;
    std::shared_ptr<ImWidgetV4::ImWidget> CreatePaletteWidget(const std::string& typeName) const;
    bool InsertWidgetIntoDocument(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const ImWidgetV4::FVector2& dropPosition);
    bool RemoveWidgetFromDocument(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool RemoveWidgetFromParent(const std::shared_ptr<ImWidgetV4::ImWidget>& parent, const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool MoveWidgetInDocument(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& newParent);
    bool InsertWidgetIntoParentAt(
        const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
        int insertIndex,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool MoveWidgetRelativeToTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone);
    void RefreshDocumentViews(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void RefreshPreview();
    void ApplySelectionToUi(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    FDocumentSnapshot CaptureDocumentSnapshot() const;
    bool ExecuteDocumentMutation(
        const std::string& commandLabel,
        const std::function<bool()>& mutation,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection = nullptr);
    void BeginDocumentGesture(
        const std::string& commandLabel,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection);
    bool CommitDocumentGesture(const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection);
    void CancelDocumentGesture();
    void MarkDocumentDirty();
    void CloseWidgetTreeContextMenu();

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::shared_ptr<EditorDocument> m_Document;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImScrollBox> m_DocumentHost;
    std::shared_ptr<ImWidgetV4::ImScrollBox> m_PreviewHost;
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> m_DesignerSurface;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
    std::shared_ptr<DocumentTreeViewBinder> m_TreeBinder;
    std::shared_ptr<ReflectionDetailsView> m_DetailsView;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_OutputText;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_WidgetTreeContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_WidgetTreeContextMenuWindow;
    CommandStack m_CommandStack;
    int m_DocumentTabIndex = -1;
    std::unique_ptr<FDocumentSnapshot> m_PendingGestureSnapshot;
    std::string m_PendingGestureLabel;
    std::weak_ptr<ImWidgetV4::ImWidget> m_PendingGestureSelection;
    json m_CopiedWidgetJson;
    bool m_bHasCopiedWidget = false;
};

} // namespace ImWidgetV4Editor
