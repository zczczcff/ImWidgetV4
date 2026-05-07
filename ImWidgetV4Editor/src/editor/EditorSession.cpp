#include "EditorSession.h"
#include "LogicalWidgetTree.h"
#include "../commands/DocumentSnapshotCommand.h"
#include "../inspector/ReflectionDetailsView.h"
#include "SelectionModel.h"
#include "../palette/WidgetPaletteDragDrop.h"
#include "../serialization/WidgetFactory.h"
#include "../serialization/WidgetSerializer.h"
#include "../tree/DocumentTreeViewBinder.h"
#include "../tree/WidgetTreeDragDrop.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <filesystem>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::vector<std::string> SplitLinesPreserveOrder(const std::string& text)
{
    std::vector<std::string> lines;
    std::stringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        lines.push_back(line);
    }

    if (lines.empty()) {
        lines.push_back(text);
    }

    return lines;
}

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

std::string StripImPrefix(const std::string& typeName)
{
    if (typeName.rfind("Im", 0) == 0 && typeName.size() > 2) {
        return typeName.substr(2);
    }

    return typeName;
}

std::string BuildTabTitleForWidget(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return "Tab";
    }

    if (!widget->GetName().empty()) {
        return widget->GetName();
    }

    return StripImPrefix(widget->GetTypeName());
}

std::shared_ptr<ReflectableObject> GetReflectableSelectionTarget(const std::shared_ptr<ImWidget>& selectedWidget)
{
    return std::dynamic_pointer_cast<ReflectableObject>(selectedWidget);
}

std::shared_ptr<ImWidget> CloneWidgetTree(const std::shared_ptr<ImWidget>& widget, std::string& outError)
{
    if (!widget) {
        outError = "No widget selected.";
        return nullptr;
    }

    FWidgetSerializationResult result =
        WidgetSerializer::DeserializeWidgetTree(WidgetSerializer::SerializeWidgetTree(widget));
    if (!result.bSuccess) {
        outError = result.ErrorMessage.empty()
            ? "Failed to clone widget tree."
            : result.ErrorMessage;
        return nullptr;
    }

    return result.Widget;
}

bool TryInsertIntoScrollBoxAt(
    const std::shared_ptr<ImScrollBox>& scrollBox,
    int insertIndex,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!scrollBox || !widget) {
        return false;
    }

    if (!scrollBox->GetContent()) {
        scrollBox->SetContent(widget);
        return true;
    }

    if (auto contentVerticalBox = std::dynamic_pointer_cast<ImVerticalBox>(scrollBox->GetContent())) {
        contentVerticalBox->InsertChild(insertIndex, widget);
        return true;
    }

    auto existingContent = scrollBox->GetContent();
    auto wrapper = std::make_shared<ImVerticalBox>();
    if (insertIndex <= 0) {
        wrapper->AddChild(widget);
        wrapper->AddChild(existingContent);
    } else {
        wrapper->AddChild(existingContent);
        wrapper->AddChild(widget);
    }
    scrollBox->SetContent(wrapper);
    return true;
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

    if (auto button = std::dynamic_pointer_cast<ImButton>(target)) {
        button->SetContent(widget);
        return true;
    }

    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(target)) {
        return TryInsertIntoScrollBoxAt(scrollBox, std::numeric_limits<int>::max(), widget);
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(target)) {
        if (!expandableBox->GetHeader()) {
            expandableBox->SetHeader(widget);
            return true;
        }

        if (!expandableBox->GetBody()) {
            expandableBox->SetBody(widget);
            expandableBox->SetExpanded(true);
            return true;
        }

        return TryInsertIntoTarget(expandableBox->GetBody(), widget, dropPosition);
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(target)) {
        const int addedIndex = tabView->AddTab(BuildTabTitleForWidget(widget), widget);
        if (addedIndex >= 0) {
            tabView->SetActiveTab(addedIndex);
            return true;
        }
        return false;
    }

    return false;
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

bool TryDuplicateInParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& sourceWidget,
    const std::shared_ptr<ImWidget>& cloneWidget)
{
    if (!parent || !sourceWidget || !cloneWidget) {
        return false;
    }

    if (auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(parent)) {
        auto* sourceSlot = dynamic_cast<ImCanvasPanelSlot*>(canvas->GetSlotForChild(sourceWidget));
        if (!sourceSlot) {
            return false;
        }

        canvas->AddChild(cloneWidget);
        auto* cloneSlot = dynamic_cast<ImCanvasPanelSlot*>(canvas->GetSlotForChild(cloneWidget));
        if (!cloneSlot) {
            return false;
        }

        cloneSlot->FromJson(sourceSlot->ToJson());
        const FVector2 duplicatedPosition(
            std::clamp(sourceSlot->GetRelativePosition().X + 0.03f, 0.0f, 0.95f),
            std::clamp(sourceSlot->GetRelativePosition().Y + 0.03f, 0.0f, 0.95f));
        cloneSlot->SetRelativePosition(duplicatedPosition);
        return true;
    }

    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        const ImSlot* sourceSlot = verticalBox->GetSlotForChild(sourceWidget);
        verticalBox->AddChild(cloneWidget);
        if (sourceSlot) {
            if (auto* cloneSlot = verticalBox->GetSlotForChild(cloneWidget)) {
                cloneSlot->FromJson(sourceSlot->ToJson());
            }
        }
        return true;
    }

    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        const ImSlot* sourceSlot = horizontalBox->GetSlotForChild(sourceWidget);
        horizontalBox->AddChild(cloneWidget);
        if (sourceSlot) {
            if (auto* cloneSlot = horizontalBox->GetSlotForChild(cloneWidget)) {
                cloneSlot->FromJson(sourceSlot->ToJson());
            }
        }
        return true;
    }

    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        return TryInsertIntoTarget(scrollBox, cloneWidget, sourceWidget->GetGeometry().Position);
    }

    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        button->SetContent(cloneWidget);
        return true;
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (expandableBox->GetHeader() == sourceWidget) {
            expandableBox->SetHeader(cloneWidget);
            return true;
        }
        if (expandableBox->GetBody() == sourceWidget) {
            expandableBox->SetBody(cloneWidget);
            expandableBox->SetExpanded(true);
            return true;
        }
        return false;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int sourceTabIndex = LogicalWidgetTree::FindTabContentIndex(tabView, sourceWidget);
        if (sourceTabIndex < 0) {
            return false;
        }

        const FTabViewItem* sourceTab = tabView->GetTab(sourceTabIndex);
        if (!sourceTab) {
            return false;
        }

        int duplicatedIndex = tabView->AddTab(sourceTab->Title + " Copy", cloneWidget);
        if (duplicatedIndex < 0) {
            return false;
        }

        tabView->SetTabEnabled(duplicatedIndex, sourceTab->bEnabled);
        tabView->SetTabClosable(duplicatedIndex, sourceTab->bClosable);
        tabView->SetTabDirty(duplicatedIndex, sourceTab->bDirty);
        tabView->SetActiveTab(duplicatedIndex);
        return true;
    }

    return false;
}

} // namespace

EditorSession::EditorSession(std::function<std::shared_ptr<ImWidget>()> createDefaultDocumentRoot)
    : m_CreateDefaultDocumentRoot(std::move(createDefaultDocumentRoot))
    , m_Document(CreateDefaultDocument())
    , m_SelectionModel(std::make_shared<SelectionModel>())
{
}

void EditorSession::SetDocumentTabBinding(
    const std::shared_ptr<ImTabView>& documentTabs,
    int documentTabIndex)
{
    m_DocumentTabs = documentTabs;
    m_DocumentTabIndex = documentTabIndex;
}

void EditorSession::BindDocumentWidgets(
    const std::shared_ptr<ImTabView>& documentTabs,
    int documentTabIndex,
    const std::shared_ptr<ImScrollBox>& documentHost,
    const std::shared_ptr<ImScrollBox>& previewHost,
    const std::shared_ptr<ImTextList>& schemaText,
    const std::shared_ptr<ImDesignerSurface>& designerSurface,
    const std::shared_ptr<ImTextOutlineView>& widgetTreeView,
    const std::shared_ptr<ReflectionDetailsView>& detailsView,
    const std::shared_ptr<ImTextList>& outputText)
{
    SetDocumentTabBinding(documentTabs, documentTabIndex);
    m_DocumentHost = documentHost;
    m_PreviewHost = previewHost;
    m_SchemaText = schemaText;
    m_DesignerSurface = designerSurface;
    m_WidgetTreeView = widgetTreeView;
    m_DetailsView = detailsView;
    m_OutputText = outputText;
    if (!m_TreeBinder) {
        m_TreeBinder = std::make_shared<DocumentTreeViewBinder>();
    }
    m_TreeBinder->Bind(m_WidgetTreeView, m_DesignerSurface, m_Document);
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
        m_WidgetTreeView->OnItemDropped.Clear();
        m_WidgetTreeView->OnItemDropped.AddLambda(
            [this](
                ImTextOutlineView& treeView,
                ImTextOutlineItem& item,
                ETextOutlineDropZone zone,
                const std::shared_ptr<FDragDropOperation>& operation,
                FVector2 position,
                bool& bHandled) {
                HandleWidgetTreeItemDropped(treeView, item, zone, operation, position, bHandled);
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
                        m_DetailsView->RebuildPreservingViewState();
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

    return OpenDocumentFromPath(dialogResult.Path);
}

bool EditorSession::OpenDocumentFromPath(const std::filesystem::path& filePath)
{
    CancelDocumentGesture();
    auto openedDocument = std::make_shared<EditorDocument>();
    std::string errorMessage;
    if (!openedDocument->Load(filePath, &errorMessage)) {
        LogStatus("Open failed: " + errorMessage);
        return false;
    }

    m_Document = std::move(openedDocument);
    m_CommandStack.Clear();
    ApplyDocumentToUi();
    LogStatus("Opened " + filePath.filename().string());
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

bool EditorSession::CutSelectedWidget()
{
    auto selectedWidget = m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;
    if (!selectedWidget) {
        LogStatus("Cut skipped: no widget selected.");
        return false;
    }

    const std::string label = selectedWidget->GetName().empty()
        ? selectedWidget->GetTypeName()
        : selectedWidget->GetTypeName() + " [" + selectedWidget->GetName() + "]";
    if (!CopySelectedWidget()) {
        return false;
    }

    if (!DeleteSelectedWidget()) {
        LogStatus("Cut failed: copied " + label + " but could not remove it.");
        return false;
    }

    LogStatus("Cut " + label);
    return true;
}

bool EditorSession::CopySelectedWidget()
{
    auto selectedWidget = m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;
    if (!selectedWidget) {
        LogStatus("Copy skipped: no widget selected.");
        return false;
    }

    m_CopiedWidgetJson = WidgetSerializer::SerializeWidgetTree(selectedWidget);
    m_bHasCopiedWidget = !m_CopiedWidgetJson.is_null();
    if (!m_bHasCopiedWidget) {
        LogStatus("Copy failed: selected widget could not be serialized.");
        return false;
    }

    const std::string label = selectedWidget->GetName().empty()
        ? selectedWidget->GetTypeName()
        : selectedWidget->GetTypeName() + " [" + selectedWidget->GetName() + "]";
    LogStatus("Copied " + label);
    return true;
}

bool EditorSession::PasteCopiedWidget()
{
    if (!m_bHasCopiedWidget || m_CopiedWidgetJson.is_null()) {
        LogStatus("Paste skipped: clipboard is empty.");
        return false;
    }

    FWidgetSerializationResult cloneResult = WidgetSerializer::DeserializeWidgetTree(m_CopiedWidgetJson);
    if (!cloneResult.bSuccess || !cloneResult.Widget) {
        LogStatus("Paste failed: " + (cloneResult.ErrorMessage.empty()
            ? std::string("clipboard widget could not be restored.")
            : cloneResult.ErrorMessage));
        return false;
    }

    std::shared_ptr<ImWidget> selectedWidget = m_DesignerSurface
        ? m_DesignerSurface->GetSelectedWidget()
        : nullptr;
    FVector2 pastePosition(24.0f, 24.0f);
    if (selectedWidget) {
        pastePosition = selectedWidget->GetGeometry().Position + FVector2(24.0f, 24.0f);
    } else if (m_Document && m_Document->GetRootWidget()) {
        pastePosition = m_Document->GetRootWidget()->GetGeometry().Position + FVector2(24.0f, 24.0f);
    }

    const bool pasted = ExecuteDocumentMutation(
        "Paste Widget",
        [this, widget = cloneResult.Widget, pastePosition]() {
            return InsertWidgetIntoDocument(widget, pastePosition);
        },
        cloneResult.Widget);
    if (!pasted) {
        LogStatus("Paste failed: current selection cannot accept the copied widget.");
        return false;
    }

    RefreshDocumentViews(cloneResult.Widget);
    LogStatus("Pasted " + cloneResult.Widget->GetTypeName());
    return true;
}

bool EditorSession::DuplicateSelectedWidget()
{
    auto selectedWidget = m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr;
    if (!selectedWidget) {
        LogStatus("Duplicate skipped: no widget selected.");
        return false;
    }

    auto parent = selectedWidget->GetParent();
    if (!parent) {
        LogStatus("Duplicate skipped: document root cannot be duplicated in place.");
        return false;
    }

    std::string cloneError;
    std::shared_ptr<ImWidget> cloneWidget = CloneWidgetTree(selectedWidget, cloneError);
    if (!cloneWidget) {
        LogStatus("Duplicate failed: " + cloneError);
        return false;
    }

    const std::string sourceLabel = selectedWidget->GetName().empty()
        ? selectedWidget->GetTypeName()
        : selectedWidget->GetTypeName() + " [" + selectedWidget->GetName() + "]";
    const bool duplicated = ExecuteDocumentMutation(
        "Duplicate Widget",
        [parent, selectedWidget, cloneWidget]() {
            return TryDuplicateInParent(parent, selectedWidget, cloneWidget);
        },
        cloneWidget);
    if (!duplicated) {
        LogStatus("Duplicate failed: unsupported parent container.");
        return false;
    }

    RefreshDocumentViews(cloneWidget);
    LogStatus("Duplicated " + sourceLabel);
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
        std::vector<std::string> items = m_OutputText->GetItems();
        const std::vector<std::string> appendedLines = SplitLinesPreserveOrder(text);
        items.insert(items.end(), appendedLines.begin(), appendedLines.end());
        constexpr std::size_t kMaxLogLines = 200;
        if (items.size() > kMaxLogLines) {
            items.erase(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(items.size() - kMaxLogLines));
        }
        m_OutputText->SetItems(items);
        if (!items.empty()) {
            m_OutputText->ScrollToItem(static_cast<int>(items.size() - 1), false);
        }
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
    if (m_TreeBinder) {
        m_TreeBinder->SetDocument(m_Document);
    }

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

    SyncSelectionState(nullptr);
}

void EditorSession::HandleDesignerSelectionChanged(
    ImDesignerSurface&,
    const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_TreeBinder) {
        m_TreeBinder->SyncSelectionFromDesigner(selectedWidget);
    }

    SyncSelectionState(selectedWidget);

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
        "Cut",
        FImageBrush(),
        {},
        true,
        false,
        [this, targetWidget]() {
            if (m_DesignerSurface) {
                m_DesignerSurface->SetSelectedWidget(targetWidget);
            }
            const bool bCut = CutSelectedWidget();
            CloseWidgetTreeContextMenu();
            (void)bCut;
        }
    });
    items.push_back(FPopupMenuItem {
        "Copy",
        FImageBrush(),
        {},
        true,
        false,
        [this, targetWidget]() {
            if (m_DesignerSurface) {
                m_DesignerSurface->SetSelectedWidget(targetWidget);
            }
            const bool bCopied = CopySelectedWidget();
            CloseWidgetTreeContextMenu();
            (void)bCopied;
        }
    });
    items.push_back(FPopupMenuItem {
        "Paste",
        FImageBrush(),
        {},
        true,
        false,
        [this, targetWidget]() {
            if (m_DesignerSurface) {
                m_DesignerSurface->SetSelectedWidget(targetWidget);
            }
            const bool bPasted = PasteCopiedWidget();
            CloseWidgetTreeContextMenu();
            (void)bPasted;
        }
    });
    items.push_back(FPopupMenuItem {"", FImageBrush(), {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Duplicate",
        FImageBrush(),
        {},
        true,
        false,
        [this, targetWidget]() {
            if (m_DesignerSurface) {
                m_DesignerSurface->SetSelectedWidget(targetWidget);
            }
            const bool bDuplicated = DuplicateSelectedWidget();
            CloseWidgetTreeContextMenu();
            (void)bDuplicated;
        }
    });
    items.push_back(FPopupMenuItem {"", FImageBrush(), {}, true, true, {}});
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

void EditorSession::HandleWidgetTreeItemDropped(
    ImTextOutlineView&,
    ImTextOutlineItem& item,
    ETextOutlineDropZone zone,
    const std::shared_ptr<FDragDropOperation>& operation,
    FVector2,
    bool& bHandled)
{
    bHandled = false;
    if (!operation || !operation->Payload || !m_Document || !m_TreeBinder) {
        return;
    }

    auto payload = std::dynamic_pointer_cast<WidgetTreeDragDropPayload>(operation->Payload);
    if (!payload || payload->WidgetId.empty()) {
        return;
    }

    auto sourceWidget = m_Document->FindWidgetById(payload->WidgetId);
    auto targetWidget = m_TreeBinder->ResolveWidget(&item);
    if (!sourceWidget || !targetWidget) {
        return;
    }

    if (sourceWidget == targetWidget || IsLogicalAncestorOf(m_Document, sourceWidget, targetWidget)) {
        LogStatus("Move rejected: cannot move a widget into itself or its descendants.");
        return;
    }

    bHandled = ExecuteDocumentMutation(
        "Move Widget",
        [this, sourceWidget, targetWidget, zone]() {
            return zone == ETextOutlineDropZone::OnItem
                ? MoveWidgetInDocument(sourceWidget, targetWidget)
                : MoveWidgetRelativeToTarget(sourceWidget, targetWidget, zone);
        },
        sourceWidget);
    if (!bHandled) {
        LogStatus("Move rejected by target container.");
        return;
    }

    RefreshDocumentViews(sourceWidget);
    LogStatus("Moved " + payload->Label);
}

void EditorSession::UpdateSelectionDetails(const std::shared_ptr<ImWidget>& selectedWidget)
{
    std::shared_ptr<ImSlot> slotTarget;
    if (selectedWidget) {
        if (auto parent = m_Document ? m_Document->FindLogicalParent(selectedWidget) : selectedWidget->GetParent()) {
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
            SyncSelectionState(selectedWidget);
            return true;
        },
        selectedWidget);
}

bool EditorSession::ApplyDocumentSnapshot(
    const json& documentJson,
    const std::string& selectionId,
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
    std::shared_ptr<ImWidget> selectedWidget = m_Document->FindWidgetById(selectionId);
    if (m_TreeBinder) {
        m_TreeBinder->SetDocument(m_Document);
    }

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

    if (m_SelectionModel) {
        m_SelectionModel->SetSelectedWidgetId(selectionId);
    }

    SyncSelectionState(selectedWidget);

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
        widget->SetName(StripImPrefix(typeName));
    }

    if (auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(widget)) {
        textBlock->SetText("Text");
    } else if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        button->SetText("Button");
    } else if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        auto header = std::make_shared<ImTextBlock>();
        header->SetName("Header");
        header->SetText("Expandable Header");

        auto body = std::make_shared<ImTextBlock>();
        body->SetName("Body");
        body->SetText("Expandable Body");

        expandableBox->SetHeader(header);
        expandableBox->SetBody(body);
        expandableBox->SetExpanded(true);
    } else if (auto checkBox = std::dynamic_pointer_cast<ImCheckBox>(widget)) {
        checkBox->SetLabel("CheckBox");
    } else if (auto editableText = std::dynamic_pointer_cast<ImEditableText>(widget)) {
        editableText->SetText("EditableText");
    } else if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        auto defaultContent = std::make_shared<ImTextBlock>();
        defaultContent->SetName("TabContent");
        defaultContent->SetText("Tab Content");
        tabView->AddTab("Tab", defaultContent);
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
        nextSelection = m_Document ? m_Document->FindLogicalParent(widget) : widget->GetParent();
    }

    const bool bRemoved = (widget == root)
        ? (m_Document->SetRootWidget(nullptr), true)
        : RemoveWidgetFromParent(
            m_Document ? m_Document->FindLogicalParent(widget) : widget->GetParent(),
            widget);

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

    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        if (button->GetContent() == widget) {
            button->SetContent(nullptr);
            return true;
        }
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (expandableBox->GetHeader() == widget) {
            expandableBox->SetHeader(nullptr);
            return true;
        }
        if (expandableBox->GetBody() == widget) {
            expandableBox->SetBody(nullptr);
            return true;
        }
    }

    return parent->RemoveChild(widget);
}

bool EditorSession::MoveWidgetInDocument(
    const std::shared_ptr<ImWidget>& widget,
    const std::shared_ptr<ImWidget>& newParent)
{
    if (!m_Document || !widget || !newParent) {
        return false;
    }

    auto root = m_Document->GetRootWidget();
    if (!root || widget == root) {
        return false;
    }

    if (widget == newParent || IsLogicalAncestorOf(m_Document, widget, newParent)) {
        return false;
    }

    const std::shared_ptr<ImWidget> oldParent = widget->GetParent();
    const std::shared_ptr<ImWidget> oldLogicalParent =
        m_Document ? m_Document->FindLogicalParent(widget) : oldParent;
    if (!oldLogicalParent) {
        return false;
    }

    if (!RemoveWidgetFromParent(oldLogicalParent, widget)) {
        return false;
    }

    const bool bInserted = TryInsertIntoTarget(newParent, widget, newParent->GetGeometry().Position);
    if (!bInserted) {
        TryInsertIntoTarget(oldLogicalParent, widget, oldLogicalParent->GetGeometry().Position);
        return false;
    }

    MarkDocumentDirty();
    return true;
}

bool EditorSession::InsertWidgetIntoParentAt(
    const std::shared_ptr<ImWidget>& parent,
    int insertIndex,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!parent || !widget) {
        return false;
    }

    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        verticalBox->InsertChild(insertIndex, widget);
        return true;
    }

    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        horizontalBox->InsertChild(insertIndex, widget);
        return true;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int insertedIndex = tabView->InsertTab(insertIndex, BuildTabTitleForWidget(widget), widget);
        if (insertedIndex >= 0) {
            tabView->SetActiveTab(insertedIndex);
            return true;
        }
        return false;
    }

    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        return TryInsertIntoScrollBoxAt(scrollBox, insertIndex, widget);
    }

    return false;
}

bool EditorSession::MoveWidgetRelativeToTarget(
    const std::shared_ptr<ImWidget>& widget,
    const std::shared_ptr<ImWidget>& targetWidget,
    ETextOutlineDropZone zone)
{
    if (!m_Document || !widget || !targetWidget || zone == ETextOutlineDropZone::OnItem) {
        return false;
    }

    auto root = m_Document->GetRootWidget();
    if (!root || widget == root || widget == targetWidget) {
        return false;
    }

    const std::shared_ptr<ImWidget> targetParent = m_Document->FindLogicalParent(targetWidget);
    const std::shared_ptr<ImWidget> oldParent = m_Document->FindLogicalParent(widget);
    if (!targetParent || !oldParent || IsLogicalAncestorOf(m_Document, widget, targetParent)) {
        return false;
    }

    int targetIndex = LogicalWidgetTree::FindLogicalChildIndex(targetParent, targetWidget);
    if (targetIndex < 0) {
        return false;
    }

    const int oldIndex = LogicalWidgetTree::FindLogicalChildIndex(oldParent, widget);
    if (oldIndex < 0) {
        return false;
    }

    int insertIndex = zone == ETextOutlineDropZone::BeforeItem ? targetIndex : (targetIndex + 1);
    if (oldParent == targetParent && oldIndex < targetIndex) {
        --insertIndex;
    }

    if (!RemoveWidgetFromParent(oldParent, widget)) {
        return false;
    }

    if (!InsertWidgetIntoParentAt(targetParent, insertIndex, widget)) {
        InsertWidgetIntoParentAt(oldParent, oldIndex, widget);
        return false;
    }

    MarkDocumentDirty();
    return true;
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

void EditorSession::SyncSelectionState(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_bSyncingSelectionState) {
        return;
    }

    m_bSyncingSelectionState = true;
    if (m_SelectionModel) {
        m_SelectionModel->SetSelectedWidget(selectedWidget, m_Document);
    }

    ApplySelectionToUi(selectedWidget);

    if (m_DetailsView) {
        m_DetailsView->RebuildPreservingViewState();
    }

    UpdateSelectionDetails(selectedWidget);
    m_bSyncingSelectionState = false;
}

void EditorSession::RefreshDocumentViews(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_TreeBinder) {
        m_TreeBinder->SetDocument(m_Document);
    }

    if (m_DesignerSurface) {
        m_DesignerSurface->SetContentRoot(m_Document ? m_Document->GetRootWidget() : nullptr);
    } else if (m_DocumentHost) {
        m_DocumentHost->SetContent(m_Document ? m_Document->GetRootWidget() : nullptr);
    }

    RefreshPreview();
    RefreshSchemaView();

    if (m_TreeBinder) {
        m_TreeBinder->RebuildFromRoot(
            m_Document ? m_Document->GetRootWidget() : nullptr,
            selectedWidget);
    }

    SyncSelectionState(selectedWidget);
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

void EditorSession::RefreshSchemaView()
{
    if (!m_SchemaText) {
        return;
    }

    if (!m_Document) {
        m_SchemaText->SetItems({"{}"});
        return;
    }

    try {
        const json documentJson = m_Document->ExportDocumentJson();
        m_SchemaText->SetItems({documentJson.dump(2)});
    } catch (const std::exception& error) {
        m_SchemaText->SetItems({std::string("Schema export failed: ") + error.what()});
    } catch (...) {
        m_SchemaText->SetItems({"Schema export failed."});
    }
}

EditorSession::FDocumentSnapshot EditorSession::CaptureDocumentSnapshot() const
{
    FDocumentSnapshot snapshot;
    if (!m_Document) {
        return snapshot;
    }

    snapshot.DocumentJson = m_Document->ExportDocumentJson();
    snapshot.SelectionId = m_SelectionModel
        ? m_SelectionModel->GetSelectedWidgetId()
        : m_Document->GetWidgetId(m_DesignerSurface ? m_DesignerSurface->GetSelectedWidget() : nullptr);
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
        if (m_SelectionModel) {
            m_SelectionModel->SetSelectedWidget(preferredSelection, m_Document);
        }
    }

    RefreshPreview();

    FDocumentSnapshot afterSnapshot = CaptureDocumentSnapshot();
    if (preferredSelection) {
        afterSnapshot.SelectionId = m_Document->GetWidgetId(preferredSelection);
    }
    m_CommandStack.PushExecuted(std::make_unique<DocumentSnapshotCommand>(
        shared_from_this(),
        commandLabel,
        beforeSnapshot.DocumentJson,
        beforeSnapshot.SelectionId,
        beforeSnapshot.bDirty,
        afterSnapshot.DocumentJson,
        afterSnapshot.SelectionId,
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
        afterSnapshot.SelectionId = m_Document->GetWidgetId(selection);
    }

    const bool bDocumentChanged =
        m_PendingGestureSnapshot->DocumentJson != afterSnapshot.DocumentJson;
    const bool bSelectionChanged =
        m_PendingGestureSnapshot->SelectionId != afterSnapshot.SelectionId;
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
        m_PendingGestureSnapshot->SelectionId,
        m_PendingGestureSnapshot->bDirty,
        afterSnapshot.DocumentJson,
        afterSnapshot.SelectionId,
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
