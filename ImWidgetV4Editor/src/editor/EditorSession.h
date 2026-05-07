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
class ImTextList;
class ImTextOutlineItem;
class ImTextOutlineView;
class ImWidget;
class ImWindow;
}

namespace ImWidgetV4Editor {

class DocumentTreeViewBinder;
class ReflectionDetailsView;
class SelectionModel;

class EditorSession : public std::enable_shared_from_this<EditorSession> {
public:
    explicit EditorSession(std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);

    void SetDocumentTabBinding(
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        int documentTabIndex);

    void BindDocumentWidgets(
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        int documentTabIndex,
        const std::shared_ptr<ImWidgetV4::ImScrollBox>& documentHost,
        const std::shared_ptr<ImWidgetV4::ImScrollBox>& previewHost,
        const std::shared_ptr<ImWidgetV4::ImTextList>& schemaText,
        const std::shared_ptr<ImWidgetV4::ImDesignerSurface>& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& widgetTreeView,
        const std::shared_ptr<ReflectionDetailsView>& detailsView,
        const std::shared_ptr<ImWidgetV4::ImTextList>& outputText);

    const std::shared_ptr<EditorDocument>& GetDocument() const { return m_Document; }
    std::string GetDocumentTabTitle() const;

    bool NewDocument();
    bool OpenDocument(ImWidgetV4::ImApplication& app);
    bool OpenDocumentFromPath(const std::filesystem::path& filePath);
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
    void UpdateDocumentFilePath(const std::filesystem::path& filePath);

    void LogStatus(const std::string& text);
    bool ApplyDocumentSnapshot(
        const json& documentJson,
        const std::string& selectionId,
        bool bDirty);
    bool ApplyReflectablePropertyChange(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const json& objectJson,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bDirtyState);
    bool ApplyWidgetInsertion(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& insertionTarget,
        const ImWidgetV4::FVector2& dropPosition,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bDirtyState);
    bool ApplyWidgetInsertionAtTreeTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bDirtyState);
    bool ApplyWidgetMoveAtTreeTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bDirtyState);
    bool ApplyWidgetMoveAtParentIndex(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
        int insertIndex,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bDirtyState);
    bool ApplyWidgetRemoval(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bDirtyState);

private:
    struct FDocumentSnapshot {
        json DocumentJson;
        std::string SelectionId;
        bool bDirty = false;
    };

    struct FReflectableGestureSnapshot {
        std::weak_ptr<ImWidgetV4::ReflectableObject> Owner;
        json BeforeJson;
        bool bBeforeDirty = false;
    };

    std::shared_ptr<EditorDocument> CreateDefaultDocument() const;
    void ApplyDocumentToUi();
    void SyncSelectionState(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void HandleDesignerSelectionChanged(
        ImWidgetV4::ImDesignerSurface& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void HandleDesignerDrop(
        ImWidgetV4::ImDesignerSurface& designerSurface,
        const std::shared_ptr<ImWidgetV4::FDragDropOperation>& operation,
        const ImWidgetV4::FVector2& position,
        bool& bHandled);
    void HandleDesignerContextMenuRequested(
        ImWidgetV4::ImDesignerSurface& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::FVector2 position);
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
    FDocumentSnapshot CaptureDocumentSnapshot(
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection = nullptr) const;
    void PushDocumentSnapshotCommand(
        std::string label,
        const FDocumentSnapshot& beforeSnapshot,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection);
    std::shared_ptr<ImWidgetV4::ImWidget> ResolveDesignerInsertionTargetAt(const ImWidgetV4::FVector2& position) const;
    std::shared_ptr<ImWidgetV4::ImWidget> CreatePaletteWidget(const std::string& typeName) const;
    bool CreatePaletteWidgetAtTreeTarget(
        const std::string& widgetTypeName,
        const std::string& label,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone);
    bool CreatePaletteWidgetAsRoot(
        const std::string& widgetTypeName,
        const std::string& label);
    bool PasteCopiedWidgetAtTreeTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone);
    bool PasteCopiedWidgetAsRoot();
    bool CanInsertWidgetAtTreeTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone) const;
    bool CanCreateRootWidget() const;
    bool CanInsertIntoParentAt(const std::shared_ptr<ImWidgetV4::ImWidget>& parent) const;
    bool InsertWidgetIntoDocument(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const ImWidgetV4::FVector2& dropPosition);
    bool InsertWidgetIntoDocumentAtTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& insertionTarget,
        const ImWidgetV4::FVector2& dropPosition);
    bool InsertWidgetAtTreeTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone);
    bool DetachWidgetFromDocument(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        std::shared_ptr<ImWidgetV4::ImWidget>* outNextSelection = nullptr);
    bool RemoveWidgetFromDocument(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool RemoveWidgetFromParent(const std::shared_ptr<ImWidgetV4::ImWidget>& parent, const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool MoveWidgetInDocument(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& newParent);
    bool MoveWidgetInDocumentAtTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& insertionTarget,
        const ImWidgetV4::FVector2& dropPosition);
    bool InsertWidgetIntoParentAt(
        const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
        int insertIndex,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool ResolveTreeInsertionParentAndIndex(
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone,
        std::shared_ptr<ImWidgetV4::ImWidget>& outParent,
        int& outInsertIndex) const;
    bool MoveWidgetRelativeToTarget(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::ETextOutlineDropZone zone);
    void RefreshDocumentViews(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void RefreshPreview();
    void RefreshSchemaView();
    void ApplySelectionToUi(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void BeginReflectableGesture(
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        const std::string& commandLabel,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection);
    bool CommitReflectableGesture(const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection);
    void CancelReflectableGesture();
    void SetDocumentDirtyState(bool bDirty);
    void MarkDocumentDirty();
    void CloseWidgetTreeContextMenu();
    void OpenStructureContextMenu(
        const std::shared_ptr<ImWidgetV4::ImWidget>& targetWidget,
        ImWidgetV4::FVector2 position);

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::shared_ptr<EditorDocument> m_Document;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImScrollBox> m_DocumentHost;
    std::shared_ptr<ImWidgetV4::ImScrollBox> m_PreviewHost;
    std::shared_ptr<ImWidgetV4::ImTextList> m_SchemaText;
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> m_DesignerSurface;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
    std::shared_ptr<DocumentTreeViewBinder> m_TreeBinder;
    std::shared_ptr<ReflectionDetailsView> m_DetailsView;
    std::shared_ptr<SelectionModel> m_SelectionModel;
    std::shared_ptr<ImWidgetV4::ImTextList> m_OutputText;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_WidgetTreeContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_WidgetTreeContextMenuWindow;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_DesignerContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_DesignerContextMenuWindow;
    CommandStack m_CommandStack;
    int m_DocumentTabIndex = -1;
    std::unique_ptr<FReflectableGestureSnapshot> m_PendingReflectableGestureSnapshot;
    std::string m_PendingGestureLabel;
    std::weak_ptr<ImWidgetV4::ImWidget> m_PendingGestureSelection;
    json m_CopiedWidgetJson;
    bool m_bHasCopiedWidget = false;
    bool m_bSyncingSelectionState = false;
};

} // namespace ImWidgetV4Editor
