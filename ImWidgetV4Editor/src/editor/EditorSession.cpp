#include "EditorSession.h"
#include "../commands/DocumentSnapshotCommand.h"
#include "../inspector/ReflectionDetailsView.h"
#include "../palette/WidgetPaletteDragDrop.h"
#include "../serialization/WidgetFactory.h"
#include "../serialization/WidgetSerializer.h"
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

std::shared_ptr<ReflectableObject> GetReflectableSelectionTarget(const std::shared_ptr<ImWidget>& selectedWidget)
{
    return std::dynamic_pointer_cast<ReflectableObject>(selectedWidget);
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
    const std::shared_ptr<ImScrollBox>& previewHost,
    const std::shared_ptr<ImDesignerSurface>& designerSurface,
    const std::shared_ptr<ImTextOutlineView>& widgetTreeView,
    const std::shared_ptr<ReflectionDetailsView>& detailsView,
    const std::shared_ptr<ImTextBlock>& outputText)
{
    m_DocumentTabs = documentTabs;
    m_DocumentTabIndex = documentTabIndex;
    m_DocumentHost = documentHost;
    m_PreviewHost = previewHost;
    m_DesignerSurface = designerSurface;
    m_WidgetTreeView = widgetTreeView;
    m_DetailsView = detailsView;
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
        m_DesignerSurface->OnTransformStarted.Clear();
        m_DesignerSurface->OnTransformStarted.AddLambda(
            [this](
                ImDesignerSurface&,
                std::shared_ptr<ImWidget> widget,
                EDesignerTransformHandle handle) {
                const std::string commandLabel =
                    handle == EDesignerTransformHandle::ResizeBottomRight
                        ? "Resize Widget"
                        : "Move Widget";
                BeginDocumentGesture(commandLabel, widget);
            });
        m_DesignerSurface->OnTransformFinished.Clear();
        m_DesignerSurface->OnTransformFinished.AddLambda(
            [this](
                ImDesignerSurface&,
                std::shared_ptr<ImWidget> widget,
                EDesignerTransformHandle,
                bool bChanged) {
                if (!bChanged) {
                    CancelDocumentGesture();
                    return;
                }

                if (CommitDocumentGesture(widget)) {
                    UpdateSelectionDetails(widget);
                    if (m_DetailsView) {
                        m_DetailsView->Rebuild();
                    }
                }
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
        m_DetailsView->OnPropertyValueCommitted.Clear();
        m_DetailsView->OnPropertyValueCommitted.AddLambda(
            [this](
                ReflectionDetailsView& detailsView,
                std::shared_ptr<ReflectableObject> owner,
                std::string propertyClassName,
                std::string propertyName,
                json value) {
                HandlePropertyValueCommitted(
                    detailsView,
                    owner,
                    propertyClassName,
                    propertyName,
                    value);
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
    CancelDocumentGesture();
    m_Document = CreateDefaultDocument();
    m_CommandStack.Clear();
    ApplyDocumentToUi();
    LogStatus("Created new document.");
    return true;
}

bool EditorSession::OpenDocument(ImApplication& app)
{
    CancelDocumentGesture();
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
    m_CommandStack.Clear();
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
    CancelDocumentGesture();
    auto selectedWidget = m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;
    if (!selectedWidget) {
        LogStatus("Delete skipped: no widget selected.");
        return false;
    }

    std::string label = selectedWidget->GetTypeName();
    if (!selectedWidget->GetName().empty()) {
        label += " [" + selectedWidget->GetName() + "]";
    }

    const bool bRemoved = ExecuteDocumentMutation(
        "Delete Widget",
        [this, selectedWidget]() {
            return RemoveWidgetFromDocument(selectedWidget);
        });
    if (!bRemoved) {
        LogStatus("Delete failed for current selection.");
        return false;
    }

    LogStatus("Deleted " + label);
    return true;
}

bool EditorSession::Undo()
{
    if (!m_CommandStack.CanUndo()) {
        LogStatus("Undo unavailable.");
        return false;
    }

    const std::string label = m_CommandStack.GetUndoLabel();
    if (!m_CommandStack.Undo()) {
        LogStatus("Undo failed.");
        return false;
    }

    LogStatus(label.empty() ? "Undo complete." : "Undo: " + label);
    return true;
}

bool EditorSession::Redo()
{
    if (!m_CommandStack.CanRedo()) {
        LogStatus("Redo unavailable.");
        return false;
    }

    const std::string label = m_CommandStack.GetRedoLabel();
    if (!m_CommandStack.Redo()) {
        LogStatus("Redo failed.");
        return false;
    }

    LogStatus(label.empty() ? "Redo complete." : "Redo: " + label);
    return true;
}

bool EditorSession::CanUndo() const
{
    return m_CommandStack.CanUndo();
}

bool EditorSession::CanRedo() const
{
    return m_CommandStack.CanRedo();
}

std::string EditorSession::GetUndoLabel() const
{
    return m_CommandStack.GetUndoLabel();
}

std::string EditorSession::GetRedoLabel() const
{
    return m_CommandStack.GetRedoLabel();
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

    RefreshPreview();

    if (m_TreeBinder) {
        m_TreeBinder->RebuildFromRoot(
            m_Document ? m_Document->GetRootWidget() : nullptr,
            nullptr);
    }

    UpdateSelectionDetails(nullptr);
    if (m_DetailsView) {
        m_DetailsView->Rebuild();
    }
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

    bHandled = ExecuteDocumentMutation(
        "Add Widget",
        [this, widget, position]() {
            return InsertWidgetIntoDocument(widget, position);
        },
        widget);
    if (!bHandled) {
        LogStatus("Drop rejected by current document root.");
        return;
    }

    RefreshDocumentViews(widget);
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
            GetReflectableSelectionTarget(selectedWidget),
            slotTarget);
    }
}

void EditorSession::HandlePropertyValueCommitted(
    ReflectionDetailsView&,
    const std::shared_ptr<ReflectableObject>& owner,
    const std::string& propertyClassName,
    const std::string& propertyName,
    const json& value)
{
    if (!owner) {
        return;
    }

    const std::shared_ptr<ImWidget> selectedWidget = m_DesignerSurface
        ? m_DesignerSurface->GetSelectedWidget()
        : nullptr;

    ExecuteDocumentMutation(
        "Edit " + propertyName,
        [this, owner, propertyClassName, propertyName, value, selectedWidget]() {
            json serialized = owner->ToJson();
            serialized["Properties"][propertyClassName + "::" + propertyName] = value;
            owner->FromJson(serialized);

            MarkDocumentDirty();
            if (m_TreeBinder) {
                m_TreeBinder->RebuildFromRoot(
                    m_Document ? m_Document->GetRootWidget() : nullptr,
                    selectedWidget);
            }
            if (m_DetailsView) {
                m_DetailsView->Rebuild();
            }
            UpdateSelectionDetails(selectedWidget);
            return true;
        },
        selectedWidget);
}

bool EditorSession::ApplyDocumentSnapshot(
    const json& documentJson,
    const std::vector<int>& selectionPath,
    bool bDirty)
{
    CancelDocumentGesture();
    if (!m_Document) {
        return false;
    }

    std::string errorMessage;
    if (!m_Document->ImportDocumentJson(documentJson, &errorMessage)) {
        LogStatus("Snapshot restore failed: " + errorMessage);
        return false;
    }

    m_Document->SetDirty(bDirty);
    std::shared_ptr<ImWidget> selectedWidget = ResolveSelectionPath(selectionPath);

    if (m_DesignerSurface) {
        m_DesignerSurface->SetContentRoot(m_Document->GetRootWidget());
    } else if (m_DocumentHost) {
        m_DocumentHost->SetContent(m_Document->GetRootWidget());
    }

    if (m_TreeBinder) {
        m_TreeBinder->RebuildFromRoot(
            m_Document ? m_Document->GetRootWidget() : nullptr,
            selectedWidget);
    }

    ApplySelectionToUi(selectedWidget);
    if (m_DetailsView) {
        m_DetailsView->Rebuild();
    }
    UpdateSelectionDetails(selectedWidget);

    if (m_DocumentTabs && m_DocumentTabIndex >= 0) {
        m_DocumentTabs->SetTabTitle(m_DocumentTabIndex, GetDocumentTabTitle());
        m_DocumentTabs->SetTabDirty(m_DocumentTabIndex, m_Document->IsDirty());
    }

    RefreshPreview();

    return true;
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

    MarkDocumentDirty();
    RefreshDocumentViews(nextSelection);
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

void EditorSession::ApplySelectionToUi(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (!m_DesignerSurface) {
        return;
    }

    if (selectedWidget) {
        m_DesignerSurface->SetSelectedWidget(selectedWidget);
    } else {
        m_DesignerSurface->ClearSelection();
    }
}

void EditorSession::RefreshDocumentViews(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_DesignerSurface) {
        m_DesignerSurface->SetContentRoot(m_Document ? m_Document->GetRootWidget() : nullptr);
    } else if (m_DocumentHost) {
        m_DocumentHost->SetContent(m_Document ? m_Document->GetRootWidget() : nullptr);
    }

    RefreshPreview();

    if (m_TreeBinder) {
        m_TreeBinder->RebuildFromRoot(
            m_Document ? m_Document->GetRootWidget() : nullptr,
            selectedWidget);
    }

    ApplySelectionToUi(selectedWidget);
    if (m_DetailsView) {
        m_DetailsView->Rebuild();
    }
    UpdateSelectionDetails(selectedWidget);
}

void EditorSession::RefreshPreview()
{
    if (!m_PreviewHost) {
        return;
    }

    if (!m_Document || !m_Document->GetRootWidget()) {
        m_PreviewHost->SetContent(nullptr);
        return;
    }

    const json previewRootJson = WidgetSerializer::SerializeWidgetTree(m_Document->GetRootWidget());
    FWidgetSerializationResult previewResult = WidgetSerializer::DeserializeWidgetTree(previewRootJson);
    if (!previewResult.bSuccess) {
        m_PreviewHost->SetContent(nullptr);
        if (!previewResult.ErrorMessage.empty()) {
            LogStatus("Preview refresh failed: " + previewResult.ErrorMessage);
        }
        return;
    }

    m_PreviewHost->SetContent(previewResult.Widget);
}

EditorSession::FDocumentSnapshot EditorSession::CaptureDocumentSnapshot() const
{
    FDocumentSnapshot snapshot;
    if (!m_Document) {
        return snapshot;
    }

    snapshot.DocumentJson = m_Document->ExportDocumentJson();
    snapshot.SelectionPath = BuildSelectionPath(
        m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr);
    snapshot.bDirty = m_Document->IsDirty();
    return snapshot;
}

bool EditorSession::ExecuteDocumentMutation(
    const std::string& commandLabel,
    const std::function<bool()>& mutation,
    const std::shared_ptr<ImWidget>& preferredSelection)
{
    if (!m_Document || !mutation) {
        return false;
    }

    const FDocumentSnapshot beforeSnapshot = CaptureDocumentSnapshot();
    if (!mutation()) {
        return false;
    }

    if (preferredSelection) {
        ApplySelectionToUi(preferredSelection);
    }

    RefreshPreview();

    FDocumentSnapshot afterSnapshot = CaptureDocumentSnapshot();
    if (preferredSelection) {
        afterSnapshot.SelectionPath = BuildSelectionPath(preferredSelection);
    }
    m_CommandStack.PushExecuted(std::make_unique<DocumentSnapshotCommand>(
        shared_from_this(),
        commandLabel,
        beforeSnapshot.DocumentJson,
        beforeSnapshot.SelectionPath,
        beforeSnapshot.bDirty,
        afterSnapshot.DocumentJson,
        afterSnapshot.SelectionPath,
        afterSnapshot.bDirty));
    return true;
}

void EditorSession::BeginDocumentGesture(
    const std::string& commandLabel,
    const std::shared_ptr<ImWidget>& preferredSelection)
{
    if (!m_Document) {
        return;
    }

    m_PendingGestureSnapshot = std::make_unique<FDocumentSnapshot>(CaptureDocumentSnapshot());
    m_PendingGestureLabel = commandLabel;
    m_PendingGestureSelection = preferredSelection;
}

bool EditorSession::CommitDocumentGesture(const std::shared_ptr<ImWidget>& preferredSelection)
{
    if (!m_Document || !m_PendingGestureSnapshot) {
        return false;
    }

    std::shared_ptr<ImWidget> selection = preferredSelection
        ? preferredSelection
        : m_PendingGestureSelection.lock();

    FDocumentSnapshot afterSnapshot = CaptureDocumentSnapshot();
    if (selection) {
        afterSnapshot.SelectionPath = BuildSelectionPath(selection);
    }

    const bool bDocumentChanged =
        m_PendingGestureSnapshot->DocumentJson != afterSnapshot.DocumentJson;
    const bool bSelectionChanged =
        m_PendingGestureSnapshot->SelectionPath != afterSnapshot.SelectionPath;
    if (!bDocumentChanged && !bSelectionChanged) {
        CancelDocumentGesture();
        return false;
    }

    MarkDocumentDirty();
    afterSnapshot.bDirty = m_Document->IsDirty();

    m_CommandStack.PushExecuted(std::make_unique<DocumentSnapshotCommand>(
        shared_from_this(),
        m_PendingGestureLabel.empty() ? "Edit Widget" : m_PendingGestureLabel,
        m_PendingGestureSnapshot->DocumentJson,
        m_PendingGestureSnapshot->SelectionPath,
        m_PendingGestureSnapshot->bDirty,
        afterSnapshot.DocumentJson,
        afterSnapshot.SelectionPath,
        afterSnapshot.bDirty));
    CancelDocumentGesture();
    return true;
}

void EditorSession::CancelDocumentGesture()
{
    m_PendingGestureSnapshot.reset();
    m_PendingGestureLabel.clear();
    m_PendingGestureSelection.reset();
}

std::vector<int> EditorSession::BuildSelectionPath(const std::shared_ptr<ImWidget>& widget) const
{
    std::vector<int> path;
    if (!m_Document || !m_Document->GetRootWidget() || !widget) {
        return path;
    }

    if (widget == m_Document->GetRootWidget()) {
        return path;
    }

    if (!BuildSelectionPathRecursive(m_Document->GetRootWidget(), widget, path)) {
        path.clear();
    }
    return path;
}

bool EditorSession::BuildSelectionPathRecursive(
    const std::shared_ptr<ImWidget>& current,
    const std::shared_ptr<ImWidget>& target,
    std::vector<int>& inOutPath) const
{
    if (!current || !target) {
        return false;
    }

    const auto& children = current->GetChildren();
    for (std::size_t childIndex = 0; childIndex < children.size(); ++childIndex) {
        const auto& child = children[childIndex];
        if (!child) {
            continue;
        }

        inOutPath.push_back(static_cast<int>(childIndex));
        if (child == target || BuildSelectionPathRecursive(child, target, inOutPath)) {
            return true;
        }
        inOutPath.pop_back();
    }

    return false;
}

std::shared_ptr<ImWidget> EditorSession::ResolveSelectionPath(const std::vector<int>& path) const
{
    if (!m_Document) {
        return nullptr;
    }

    std::shared_ptr<ImWidget> current = m_Document->GetRootWidget();
    if (!current) {
        return nullptr;
    }

    for (int childIndex : path) {
        const auto& children = current->GetChildren();
        if (childIndex < 0 || childIndex >= static_cast<int>(children.size())) {
            return nullptr;
        }

        current = children[childIndex];
        if (!current) {
            return nullptr;
        }
    }

    return current;
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
