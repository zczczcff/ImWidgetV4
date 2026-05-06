#pragma once

#include "EditorDocument.h"

#include <imwidgetv4/core/Application.h>
#include <functional>
#include <memory>
#include <string>

namespace ImWidgetV4 {
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

class EditorSession {
public:
    explicit EditorSession(std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);

    void BindDocumentWidgets(
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        int documentTabIndex,
        const std::shared_ptr<ImWidgetV4::ImScrollBox>& documentHost,
        const std::shared_ptr<ImWidgetV4::ImDesignerSurface>& designerSurface,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& widgetTreeView,
        const std::shared_ptr<ReflectionDetailsView>& detailsView,
        const std::shared_ptr<ImWidgetV4::ImTextBlock>& selectionText,
        const std::shared_ptr<ImWidgetV4::ImTextBlock>& outputText);

    const std::shared_ptr<EditorDocument>& GetDocument() const { return m_Document; }
    std::string GetDocumentTabTitle() const;

    bool NewDocument();
    bool OpenDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocumentAs(ImWidgetV4::ImApplication& app);
    bool DeleteSelectedWidget();

    void LogStatus(const std::string& text);

private:
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
    void UpdateSelectionDetails(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    std::filesystem::path ResolveDialogDirectory() const;
    std::shared_ptr<ImWidgetV4::ImWidget> CreatePaletteWidget(const std::string& typeName) const;
    bool InsertWidgetIntoDocument(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const ImWidgetV4::FVector2& dropPosition);
    bool RemoveWidgetFromDocument(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    bool RemoveWidgetFromParent(const std::shared_ptr<ImWidgetV4::ImWidget>& parent, const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    void RefreshDocumentViews(const std::shared_ptr<ImWidgetV4::ImWidget>& selectedWidget);
    void MarkDocumentDirty();
    void CloseWidgetTreeContextMenu();

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::shared_ptr<EditorDocument> m_Document;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImScrollBox> m_DocumentHost;
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> m_DesignerSurface;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
    std::shared_ptr<DocumentTreeViewBinder> m_TreeBinder;
    std::shared_ptr<ReflectionDetailsView> m_DetailsView;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_SelectionText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_OutputText;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_WidgetTreeContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_WidgetTreeContextMenuWindow;
    int m_DocumentTabIndex = -1;
};

} // namespace ImWidgetV4Editor
