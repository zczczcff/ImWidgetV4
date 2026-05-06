#include "EditorSession.h"
#include "../inspector/ReflectionDetailsView.h"

#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
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
    const std::shared_ptr<ReflectionDetailsView>& detailsView,
    const std::shared_ptr<ImTextBlock>& selectionText,
    const std::shared_ptr<ImTextBlock>& outputText)
{
    m_DocumentTabs = documentTabs;
    m_DocumentTabIndex = documentTabIndex;
    m_DocumentHost = documentHost;
    m_DesignerSurface = designerSurface;
    m_DetailsView = detailsView;
    m_SelectionText = selectionText;
    m_OutputText = outputText;
    if (m_DesignerSurface) {
        m_DesignerSurface->OnSelectionChanged.Clear();
        m_DesignerSurface->OnSelectionChanged.AddLambda(
            [this](ImDesignerSurface& designerSurfaceRef, std::shared_ptr<ImWidget> selectedWidget) {
                HandleDesignerSelectionChanged(designerSurfaceRef, selectedWidget);
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

    UpdateSelectionDetails(nullptr);
}

void EditorSession::HandleDesignerSelectionChanged(
    ImDesignerSurface&,
    const std::shared_ptr<ImWidget>& selectedWidget)
{
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

void EditorSession::UpdateSelectionDetails(const std::shared_ptr<ImWidget>& selectedWidget)
{
    if (m_DetailsView) {
        m_DetailsView->SetTarget(std::dynamic_pointer_cast<ReflectableObject>(selectedWidget));
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
    m_SelectionText->SetText(text);
}

std::filesystem::path EditorSession::ResolveDialogDirectory() const
{
    if (m_Document && m_Document->HasFilePath()) {
        return m_Document->GetFilePath().parent_path();
    }

    return std::filesystem::current_path();
}

} // namespace ImWidgetV4Editor
