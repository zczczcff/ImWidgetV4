#include "EditorSession.h"
#include "../inspector/ReflectionDetailsView.h"
#include "../palette/WidgetPaletteDragDrop.h"
#include "../serialization/WidgetFactory.h"
#include "../tree/DocumentTreeViewBinder.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <filesystem>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::string BuildDisplayName(const std::shared_ptr<EditorDocument>& document)
{
    if (!document) {
        return "Main.ui";
    }

    std::string baseName;
    if (document->HasFilePath()) {
        baseName = document->GetFilePath().stem().string();
    } else {
        baseName = document->GetDisplayTitle();
    }

    if (baseName.empty()) {
        baseName = "Untitled";
    }

    if (baseName.size() < 3 || baseName.substr(baseName.size() - 3) != ".ui") {
        baseName += ".ui";
    }

    return baseName;
}

std::vector<FFileDialogFilter> BuildDocumentFilters()
{
    return {
        FFileDialogFilter {"ImWidgetV4 UI", {"*.ui.json", "*.json"}},
        FFileDialogFilter {"JSON", {"*.json"}}
    };
}

bool TryInsertIntoTarget(
    const std::shared_ptr<ImWidget>& target,
    const std::shared_ptr<ImWidget>& widget,
    const FVector2& dropPosition)
{
    if (!target || !widget) {
        return false;
    }

    if (auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(target)) {
        const FGeometry geometry = canvas->GetGeometry();
        FVector2 relativePosition(0.05f, 0.05f);
        if (geometry.Size.X > 0.0f && geometry.Size.Y > 0.0f) {
            relativePosition = FVector2(
                std::clamp((dropPosition.X - geometry.Position.X) / geometry.Size.X, 0.0f, 0.95f),
                std::clamp((dropPosition.Y - geometry.Position.Y) / geometry.Size.Y, 0.0f, 0.95f));
        }
        canvas->AddChildAt(widget, relativePosition);
        return true;
    }

    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(target)) {
        verticalBox->AddChild(widget);
        return true;
    }

    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(target)) {
        horizontalBox->AddChild(widget);
        return true;
    }

    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(target)) {
        if (!scrollBox->GetContent()) {
            scrollBox->SetContent(widget);
            return true;
        }

        if (auto contentVerticalBox = std::dynamic_pointer_cast<ImVerticalBox>(scrollBox->GetContent())) {
            contentVerticalBox->AddChild(widget);
            return true;
        }

        auto wrapper = std::make_shared<ImVerticalBox>();
        wrapper->AddChild(scrollBox->GetContent());
        wrapper->AddChild(widget);
        scrollBox->SetContent(wrapper);
        return true;
    }

    return false;
}

} // namespace

EditorSession::EditorSession(std::function<std::shared_ptr<ImWidget>()> createDefaultDocumentRoot)
    : m_CreateDefaultDocumentRoot(std::move(createDefaultDocumentRoot))
    , m_Document(CreateDefaultDocument())
{
}

void EditorSession::BindDocumentWidgets(
    const std::shared_ptr<ImTabView>& documentTabs,
    int documentTabIndex,
    const std::shared_ptr<ImScrollBox>& documentHost,
    const std::shared_ptr<ImDesignerSurface>& designerSurface,
    const std::shared_ptr<ImTextOutlineView>& widgetTreeView,
    const std::shared_ptr<ReflectionDetailsView>& detailsView,
    const std::shared_ptr<ImTextBlock>& selectionText,
    const std::shared_ptr<ImTextBlock>& outputText)
{
    m_DocumentTabs = documentTabs;
    m_DocumentTabIndex = documentTabIndex;
    m_DocumentHost = documentHost;
    m_DesignerSurface = designerSurface;
    m_WidgetTreeView = widgetTreeView;
    m_DetailsView = detailsView;
    m_SelectionText = selectionText;
    m_OutputText = outputText;
    if (!m_TreeBinder) {
        m_TreeBinder = std::make_shared<DocumentTreeViewBinder>();
    }
    m_TreeBinder->Bind(m_WidgetTreeView, m_DesignerSurface);
    if (m_WidgetTreeView) {
        m_WidgetTreeView->OnItemContextMenuRequested.Clear();
        m_WidgetTreeView->OnItemContextMenuRequested.AddLambda(
            [this](ImTextOutlineView& treeView, ImTextOutlineItem& item, FVector2 position) {
                HandleWidgetTreeContextMenuRequested(treeView, item, position);
            });
        m_WidgetTreeView->OnDeleteRequested.Clear();
        m_WidgetTreeView->OnDeleteRequested.AddLambda(
            [this](ImTextOutlineView&, ImTextOutlineItem* item) {
                if (!m_TreeBinder || item == nullptr) {
                    LogStatus("Delete skipped: no tree item selected.");
                    return;
                }

                auto widget = m_TreeBinder->ResolveWidget(item);
                if (!widget) {
                    LogStatus("Delete failed: tree item no longer maps to a widget.");
                    return;
                }

                if (m_DesignerSurface) {
                    m_DesignerSurface->SetSelectedWidget(widget);
                }
                DeleteSelectedWidget();
            });
    }
    if (m_DesignerSurface) {
        m_DesignerSurface->OnSelectionChanged.Clear();
        m_DesignerSurface->OnSelectionChanged.AddLambda(
            [this](ImDesignerSurface& designerSurfaceRef, std::shared_ptr<ImWidget> selectedWidget) {
                HandleDesignerSelectionChanged(designerSurfaceRef, selectedWidget);
            });
        m_DesignerSurface->OnDeleteRequested.Clear();
        m_DesignerSurface->OnDeleteRequested.AddLambda(
            [this](ImDesignerSurface&) {
                DeleteSelectedWidget();
            });
        m_DesignerSurface->OnDropReceived.Clear();
        m_DesignerSurface->OnDropReceived.AddLambda(
            [this](ImDesignerSurface& designerSurfaceRef,
                   const std::shared_ptr<FDragDropOperation>& operation,
                   const FVector2& position,
                   bool& bHandled) {
                HandleDesignerDrop(designerSurfaceRef, operation, position, bHandled);
            });
    }
    if (m_DetailsView) {
        m_DetailsView->OnPropertiesChanged.Clear();
        m_DetailsView->OnPropertiesChanged.AddLambda(
            [this](ReflectionDetailsView&) {
                MarkDocumentDirty();
                auto selectedWidget = m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;
                if (m_TreeBinder) {
                    m_TreeBinder->RebuildFromRoot(
                        m_Document ? m_Document->GetRootWidget() : nullptr,
                        selectedWidget);
                }
                UpdateSelectionDetails(selectedWidget);
            });
    }
    ApplyDocumentToUi();
    LogStatus("Ready.");
}

std::string EditorSession::GetDocumentTabTitle() const
{
    return BuildDisplayName(m_Document);
}

bool EditorSession::NewDocument()
{
    m_Document = CreateDefaultDocument();
    ApplyDocumentToUi();
    LogStatus("Created new document.");
    return true;
}

bool EditorSession::OpenDocument(ImApplication& app)
{
    FOpenFileDialogOptions options;
    options.Title = "Open UI Document";
    options.InitialDirectory = ResolveDialogDirectory();
    options.Filters = BuildDocumentFilters();
    options.DefaultFilterIndex = 0;

    const FPathDialogResult dialogResult = app.OpenFileDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Error) {
            LogStatus("Open failed: " + dialogResult.ErrorMessage);
        }
        return false;
    }

    auto openedDocument = std::make_shared<EditorDocument>();
    std::string errorMessage;
    if (!openedDocument->Load(dialogResult.Path, &errorMessage)) {
        LogStatus("Open failed: " + errorMessage);
        return false;
    }

    m_Document = std::move(openedDocument);
    ApplyDocumentToUi();
    LogStatus("Opened " + dialogResult.Path.filename().string());
    return true;
}

bool EditorSession::SaveDocument(ImApplication& app)
{
    if (!m_Document) {
        LogStatus("Save failed: no active document.");
        return false;
    }

    if (!m_Document->HasFilePath()) {
        return SaveDocumentAs(app);
    }

    std::string errorMessage;
    if (!m_Document->Save(&errorMessage)) {
        LogStatus("Save failed: " + errorMessage);
        return false;
    }

    ApplyDocumentToUi();
    LogStatus("Saved " + m_Document->GetFilePath().filename().string());
    return true;
}

bool EditorSession::SaveDocumentAs(ImApplication& app)
{
    if (!m_Document) {
        LogStatus("Save failed: no active document.");
        return false;
    }

    FSaveFileDialogOptions options;
    options.Title = "Save UI Document";
    options.InitialDirectory = ResolveDialogDirectory();
    options.DefaultFileName = GetDocumentTabTitle();
    options.DefaultExtension = "json";
    options.Filters = BuildDocumentFilters();
    options.DefaultFilterIndex = 0;
    options.bPromptOverwrite = true;

    const FPathDialogResult dialogResult = app.SaveFileDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Error) {
            LogStatus("Save failed: " + dialogResult.ErrorMessage);
        }
        return false;
    }

    std::string errorMessage;
    if (!m_Document->SaveAs(dialogResult.Path, &errorMessage)) {
        LogStatus("Save failed: " + errorMessage);
        return false;
    }

    ApplyDocumentToUi();
    LogStatus("Saved " + dialogResult.Path.filename().string());
    return true;
}

bool EditorSession::DeleteSelectedWidget()
{
    auto selectedWidget = m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;
    if (!selectedWidget) {
        LogStatus("Delete skipped: no widget selected.");
        return false;
    }

    const bool bRemoved = RemoveWidgetFromDocument(selectedWidget);
    if (!bRemoved) {
        LogStatus("Delete failed for current selection.");
        return false;
    }

    std::string label = selectedWidget->GetTypeName();
    if (!selectedWidget->GetName().empty()) {
        label += " [" + selectedWidget->GetName() + "]";
    }
    LogStatus("Deleted " + label);
    return true;
}

void EditorSession::LogStatus(const std::string& text)
{
    if (m_OutputText) {
        m_OutputText->SetText(text);
    }
}

std::shared_ptr<EditorDocument> EditorSession::CreateDefaultDocument() const
{
    auto document = std::make_shared<EditorDocument>();
    if (m_CreateDefaultDocumentRoot) {
        document->NewDocument(m_CreateDefaultDocumentRoot(), "Main");
    } else {
        document->NewDocument(nullptr, "Main");
    }
    return document;
}

void EditorSession::ApplyDocumentToUi()
{
    if (m_DesignerSurface) {
        m_DesignerSurface->SetContentRoot(m_Document ? m_Document->GetRootWidget() : nullptr);
        m_DesignerSurface->ClearSelection();
    } else if (m_DocumentHost) {
        m_DocumentHost->SetContent(m_Document ? m_Document->GetRootWidget() : nullptr);
    }

    if (m_DocumentTabs && m_DocumentTabIndex >= 0) {
        m_DocumentTabs->SetTabTitle(m_DocumentTabIndex, GetDocumentTabTitle());
        m_DocumentTabs->SetTabDirty(m_DocumentTabIndex, m_Document && m_Document->IsDirty());
    }

    if (m_TreeBinder) {
        m_TreeBinder->RebuildFromRoot(
            m_Document ? m_Document->GetRootWidget() : nullptr,
            nullptr);
    }

    UpdateSelectionDetails(nullptr);
}

void EditorSession::HandleDesignerSelectionChanged(
    ImDesignerSurface&,
    const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_TreeBinder) {
        m_TreeBinder->SyncSelectionFromDesigner(selectedWidget);
    }

    UpdateSelectionDetails(selectedWidget);

    if (!selectedWidget) {
        LogStatus("Selection cleared.");
        return;
    }

    std::string label = selectedWidget->GetTypeName();
    if (!selectedWidget->GetName().empty()) {
        label += " [" + selectedWidget->GetName() + "]";
    }
    LogStatus("Selected " + label);
}

void EditorSession::HandleDesignerDrop(
    ImDesignerSurface&,
    const std::shared_ptr<FDragDropOperation>& operation,
    const FVector2& position,
    bool& bHandled)
{
    if (!operation || !operation->Payload) {
        return;
    }

    auto payload = std::dynamic_pointer_cast<WidgetPalettePayload>(operation->Payload);
    if (!payload) {
        return;
    }

    auto widget = CreatePaletteWidget(payload->WidgetTypeName);
    if (!widget) {
        LogStatus("Create failed: unsupported widget type " + payload->WidgetTypeName);
        return;
    }

    bHandled = InsertWidgetIntoDocument(widget, position);
    if (!bHandled) {
        LogStatus("Drop rejected by current document root.");
        return;
    }

    MarkDocumentDirty();
    RefreshDocumentViews(widget);
    if (m_DesignerSurface) {
        m_DesignerSurface->SetSelectedWidget(widget);
    }
    LogStatus("Created " + payload->Label);
}

void EditorSession::HandleWidgetTreeContextMenuRequested(
    ImTextOutlineView&,
    ImTextOutlineItem& item,
    FVector2 position)
{
    CloseWidgetTreeContextMenu();

    if (!m_TreeBinder || !m_WidgetTreeView) {
        return;
    }

    auto targetWidget = m_TreeBinder->ResolveWidget(&item);
    if (!targetWidget || !m_WidgetTreeView->GetApplication()) {
        return;
    }

    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle style = popupMenu->GetStyle();
    style.CornerRadius = 6.0f;
    popupMenu->SetStyle(style);

    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {
        "Delete",
        FImageBrush(),
        {},
        true,
        false,
        [this, targetWidget]() {
            if (m_DesignerSurface) {
                m_DesignerSurface->SetSelectedWidget(targetWidget);
            }
            const bool bRemoved = DeleteSelectedWidget();
            CloseWidgetTreeContextMenu();
            (void)bRemoved;
        }
    });

    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([this](ImPopupMenu&, int) {
        CloseWidgetTreeContextMenu();
    });

    FPopupOptions popupOptions;
    popupOptions.Title = "WidgetTreeContextMenu";
    popupOptions.Position = position;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_WidgetTreeContextMenu = popupMenu;
    m_WidgetTreeContextMenuWindow =
        m_WidgetTreeView->GetApplication()->GetWindowManager().CreatePopup(popupOptions);
}

void EditorSession::UpdateSelectionDetails(const std::shared_ptr<ImWidget>& selectedWidget)
{
    std::shared_ptr<ImSlot> slotTarget;
    if (selectedWidget) {
        if (auto parent = selectedWidget->GetParent()) {
            if (auto panelParent = std::dynamic_pointer_cast<ImPanelWidget>(parent)) {
                ImSlot* slot = panelParent->GetSlotForChild(selectedWidget);
                if (slot) {
                    slotTarget = std::shared_ptr<ImSlot>(
                        panelParent,
                        slot);
                }
            }
        }
    }

    if (m_DetailsView) {
        m_DetailsView->SetTargets(
            std::dynamic_pointer_cast<ReflectableObject>(selectedWidget),
            slotTarget);
    }

    if (!m_SelectionText) {
        return;
    }

    if (!selectedWidget) {
        m_SelectionText->SetText("No widget selected.\nClick a widget in the designer surface to inspect its reflected properties next.");
        return;
    }

    std::string text = "Type: " + selectedWidget->GetTypeName();
    text += "\nName: " + (selectedWidget->GetName().empty() ? std::string("<unnamed>") : selectedWidget->GetName());
    const FGeometry geometry = selectedWidget->GetGeometry();
    text += "\nPosition: (" + std::to_string(static_cast<int>(geometry.Position.X)) + ", " + std::to_string(static_cast<int>(geometry.Position.Y)) + ")";
    text += "\nSize: (" + std::to_string(static_cast<int>(geometry.Size.X)) + ", " + std::to_string(static_cast<int>(geometry.Size.Y)) + ")";
    if (auto parent = selectedWidget->GetParent()) {
        text += "\nParent: " + parent->GetTypeName();
    }
    m_SelectionText->SetText(text);
}

std::filesystem::path EditorSession::ResolveDialogDirectory() const
{
    if (m_Document && m_Document->HasFilePath()) {
        return m_Document->GetFilePath().parent_path();
    }

    return std::filesystem::current_path();
}

std::shared_ptr<ImWidget> EditorSession::CreatePaletteWidget(const std::string& typeName) const
{
    auto widget = WidgetFactory::Get().CreateWidget(typeName);
    if (!widget) {
        return nullptr;
    }

    if (widget->GetName().empty()) {
        std::string baseName = typeName;
        if (baseName.rfind("Im", 0) == 0) {
            baseName = baseName.substr(2);
        }
        widget->SetName(baseName);
    }

    if (auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(widget)) {
        textBlock->SetText("Text");
    } else if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        button->SetText("Button");
    } else if (auto checkBox = std::dynamic_pointer_cast<ImCheckBox>(widget)) {
        checkBox->SetLabel("CheckBox");
    } else if (auto editableText = std::dynamic_pointer_cast<ImEditableText>(widget)) {
        editableText->SetText("EditableText");
    }

    return widget;
}

bool EditorSession::InsertWidgetIntoDocument(
    const std::shared_ptr<ImWidget>& widget,
    const FVector2& dropPosition)
{
    if (!m_Document || !widget) {
        return false;
    }

    auto root = m_Document->GetRootWidget();
    if (!root) {
        m_Document->SetRootWidget(widget);
        if (m_DesignerSurface) {
            m_DesignerSurface->SetContentRoot(widget);
        }
        return true;
    }

    std::shared_ptr<ImWidget> insertionTarget =
        m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;

    while (insertionTarget) {
        if (TryInsertIntoTarget(insertionTarget, widget, dropPosition)) {
            return true;
        }

        if (insertionTarget == root) {
            break;
        }

        insertionTarget = insertionTarget->GetParent();
    }

    return TryInsertIntoTarget(root, widget, dropPosition);
}

bool EditorSession::RemoveWidgetFromDocument(const std::shared_ptr<ImWidget>& widget)
{
    if (!m_Document || !widget) {
        return false;
    }

    auto root = m_Document->GetRootWidget();
    if (!root) {
        return false;
    }

    std::shared_ptr<ImWidget> nextSelection;
    if (m_DesignerSurface && m_DesignerSurface->GetSelectedWidget() == widget) {
        nextSelection = widget->GetParent();
    }

    const bool bRemoved = (widget == root)
        ? (m_Document->SetRootWidget(nullptr), true)
        : RemoveWidgetFromParent(widget->GetParent(), widget);

    if (!bRemoved) {
        return false;
    }

    if (m_DesignerSurface && m_DesignerSurface->GetSelectedWidget() == widget) {
        m_DesignerSurface->ClearSelection();
    }

    MarkDocumentDirty();
    RefreshDocumentViews(nextSelection);
    if (m_DesignerSurface) {
        m_DesignerSurface->SetSelectedWidget(nextSelection);
    }
    return true;
}

bool EditorSession::RemoveWidgetFromParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!parent || !widget) {
        return false;
    }

    if (auto panelParent = std::dynamic_pointer_cast<ImPanelWidget>(parent)) {
        return panelParent->RemoveChild(widget);
    }

    if (auto userWidget = std::dynamic_pointer_cast<ImUserWidget>(parent)) {
        if (userWidget->GetRootWidget() == widget) {
            userWidget->SetRootWidget(nullptr);
            return true;
        }
    }

    return parent->RemoveChild(widget);
}

void EditorSession::RefreshDocumentViews(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_DesignerSurface) {
        m_DesignerSurface->SetContentRoot(m_Document ? m_Document->GetRootWidget() : nullptr);
    } else if (m_DocumentHost) {
        m_DocumentHost->SetContent(m_Document ? m_Document->GetRootWidget() : nullptr);
    }

    if (m_TreeBinder) {
        m_TreeBinder->RebuildFromRoot(
            m_Document ? m_Document->GetRootWidget() : nullptr,
            selectedWidget);
    }

    UpdateSelectionDetails(selectedWidget);
}

void EditorSession::MarkDocumentDirty()
{
    if (m_Document) {
        m_Document->SetDirty(true);
    }

    if (m_DocumentTabs && m_DocumentTabIndex >= 0) {
        m_DocumentTabs->SetTabTitle(m_DocumentTabIndex, GetDocumentTabTitle());
        m_DocumentTabs->SetTabDirty(m_DocumentTabIndex, m_Document && m_Document->IsDirty());
    }
}

void EditorSession::CloseWidgetTreeContextMenu()
{
    if (m_WidgetTreeContextMenuWindow && m_WidgetTreeView && m_WidgetTreeView->GetApplication()) {
        m_WidgetTreeView->GetApplication()->GetWindowManager().CloseWindow(m_WidgetTreeContextMenuWindow);
    }

    m_WidgetTreeContextMenuWindow.reset();
    m_WidgetTreeContextMenu.reset();
}

} // namespace ImWidgetV4Editor
