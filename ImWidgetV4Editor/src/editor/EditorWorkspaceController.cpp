#include "EditorWorkspaceController.h"

#include "EditorSession.h"
#include "EditorPaths.h"
#include "EditorShellHost.h"
#include "InputDialog.h"
#include "../inspector/ReflectionDetailsView.h"
#include "../inspector/PropertyEditorWidgets.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#include <Shellapi.h>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <system_error>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
namespace {

std::shared_ptr<ImTextBlock> MakePanelTitle(const std::string& text)
{
    auto title = std::make_shared<ImTextBlock>();
    title->SetText(text);
    title->SetFontSize(18.0f);
    title->SetTextColor(FColor::FromBytes(238, 242, 247));
    return title;
}

std::shared_ptr<ImTextBlock> MakePanelBody(const std::string& text, float fontSize = 14.0f)
{
    auto body = std::make_shared<ImTextBlock>();
    body->SetText(text);
    body->SetWrapText(false);
    body->SetFontSize(fontSize);
    body->SetTextColor(FColor::FromBytes(180, 190, 204));
    return body;
}

std::shared_ptr<ImVerticalBox> MakeSimplePanel(const std::string& title, const std::string& bodyText)
{
    auto panel = std::make_shared<ImVerticalBox>();
    panel->SetSpacing(10.0f);
    panel->AddChild(MakePanelTitle(title), FMargin(14.0f, 14.0f, 14.0f, 14.0f));
    panel->AddChild(MakePanelBody(bodyText), FMargin(14.0f, 0.0f, 14.0f, 14.0f));
    return panel;
}

std::shared_ptr<ImScrollBox> CreateDocumentHost()
{
    auto documentHost = std::make_shared<ImScrollBox>();
    FScrollBoxStyle scrollStyle = documentHost->GetStyle();
    scrollStyle.BackgroundColor = FColor::FromBytes(18, 23, 29);
    scrollStyle.BorderThickness = 0.0f;
    scrollStyle.CornerRadius = 0.0f;
    scrollStyle.Padding = FMargin(0.0f);
    documentHost->SetStyle(scrollStyle);
    return documentHost;
}

std::shared_ptr<ImScrollBox> CreatePreviewHost()
{
    auto previewHost = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style = previewHost->GetStyle();
    style.BackgroundColor = FColor::FromBytes(18, 23, 29);
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.Padding = FMargin(0.0f);
    previewHost->SetStyle(style);
    return previewHost;
}

std::shared_ptr<ImTextList> CreateSchemaText()
{
    auto schemaText = std::make_shared<ImTextList>();
    FTextListStyle style = schemaText->GetStyle();
    style.BackgroundColor = FColor::FromBytes(18, 23, 29);
    style.BorderColor = FColor::Transparent;
    style.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    style.TextColor = FColor::FromBytes(196, 205, 217);
    style.SelectionBackgroundColor = FColor::FromBytes(72, 104, 146, 148);
    style.Padding = FMargin(12.0f);
    style.MinDesiredSize = FVector2(0.0f, 180.0f);
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.LineSpacing = 1.1f;
    schemaText->SetStyle(style);
    schemaText->SetItems({"{}"});
    return schemaText;
}

std::shared_ptr<ImTextList> CreateCodePreviewText()
{
    auto previewText = std::make_shared<ImTextList>();
    FTextListStyle style = previewText->GetStyle();
    style.BackgroundColor = FColor::FromBytes(18, 23, 29);
    style.BorderColor = FColor::Transparent;
    style.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    style.TextColor = FColor::FromBytes(196, 205, 217);
    style.SelectionBackgroundColor = FColor::FromBytes(72, 104, 146, 148);
    style.Padding = FMargin(12.0f);
    style.MinDesiredSize = FVector2(0.0f, 180.0f);
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.LineSpacing = 1.1f;
    previewText->SetStyle(style);
    previewText->SetItems({"// Code preview unavailable."});
    return previewText;
}

FTabViewStyle CreateWorkspaceTabStyle()
{
    FTabViewStyle tabStyle;
    tabStyle.Padding = FMargin(0.0f);
    tabStyle.TabHeight = 28.0f;
    tabStyle.TabMinWidth = 110.0f;
    tabStyle.TabSpacing = 0.0f;
    tabStyle.BorderThickness = 0.0f;
    tabStyle.CornerRadius = 0.0f;
    tabStyle.TabStripBackgroundColor = FColor::FromBytes(27, 33, 41);
    tabStyle.BackgroundColor = FColor::FromBytes(18, 23, 29);
    tabStyle.ActiveTabColor = FColor::FromBytes(63, 90, 128);
    return tabStyle;
}

bool ShouldSkipWorkspaceEntry(const std::filesystem::directory_entry& entry)
{
    const std::string name = entry.path().filename().string();
    return name == ".git" ||
           name == ".vs" ||
           name == "build" ||
           name == "out" ||
           name == "bin";
}

bool IsSupportedWorkspaceDocument(const std::filesystem::path& filePath)
{
    const std::string extension = filePath.extension().string();
    return extension == ".json" || extension == ".ui" || extension == ".txt";
}

bool IsPathWithinRoot(const std::filesystem::path& candidate, const std::filesystem::path& root)
{
    if (candidate.empty() || root.empty()) {
        return false;
    }

    std::error_code error;
    const std::filesystem::path normalizedCandidate = std::filesystem::weakly_canonical(candidate, error);
    const std::filesystem::path resolvedCandidate = error ? candidate.lexically_normal() : normalizedCandidate;
    error.clear();
    const std::filesystem::path normalizedRoot = std::filesystem::weakly_canonical(root, error);
    const std::filesystem::path resolvedRoot = error ? root.lexically_normal() : normalizedRoot;

    auto rootIt = resolvedRoot.begin();
    auto candidateIt = resolvedCandidate.begin();
    for (; rootIt != resolvedRoot.end() && candidateIt != resolvedCandidate.end(); ++rootIt, ++candidateIt) {
        if (*rootIt != *candidateIt) {
            return false;
        }
    }

    return rootIt == resolvedRoot.end();
}

std::filesystem::path NormalizePathForComparison(const std::filesystem::path& path)
{
    if (path.empty()) {
        return {};
    }

    std::error_code error;
    const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, error);
    return error ? path.lexically_normal() : canonicalPath;
}

bool AreEquivalentPaths(const std::filesystem::path& left, const std::filesystem::path& right)
{
    if (left.empty() || right.empty()) {
        return false;
    }

    return NormalizePathForComparison(left) == NormalizePathForComparison(right);
}

std::filesystem::path BuildUniqueChildPath(
    const std::filesystem::path& directoryPath,
    const std::string& baseName)
{
    std::filesystem::path candidate = directoryPath / baseName;
    if (!std::filesystem::exists(candidate)) {
        return candidate;
    }

    for (int suffix = 2; suffix < 10000; ++suffix) {
        std::ostringstream builder;
        builder << baseName << suffix;
        candidate = directoryPath / builder.str();
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

std::filesystem::path BuildUniqueChildFilePath(
    const std::filesystem::path& directoryPath,
    const std::string& baseStem,
    const std::string& compoundExtension)
{
    if (directoryPath.empty() || baseStem.empty()) {
        return {};
    }

    std::filesystem::path candidate = directoryPath / (baseStem + compoundExtension);
    if (!std::filesystem::exists(candidate)) {
        return candidate;
    }

    for (int suffix = 2; suffix < 10000; ++suffix) {
        std::ostringstream builder;
        builder << baseStem << suffix << compoundExtension;
        candidate = directoryPath / builder.str();
        if (!std::filesystem::exists(candidate)) {
            return candidate;
        }
    }

    return {};
}

bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix)
{
    if (suffix.size() > value.size()) {
        return false;
    }

    const std::size_t offset = value.size() - suffix.size();
    for (std::size_t index = 0; index < suffix.size(); ++index) {
        const char left = static_cast<char>(std::tolower(static_cast<unsigned char>(value[offset + index])));
        const char right = static_cast<char>(std::tolower(static_cast<unsigned char>(suffix[index])));
        if (left != right) {
            return false;
        }
    }

    return true;
}

} // namespace

EditorWorkspaceController::EditorWorkspaceController(
    std::function<std::shared_ptr<ImWidget>()> createDefaultDocumentRoot)
    : m_CreateDefaultDocumentRoot(std::move(createDefaultDocumentRoot))
{
}

void EditorWorkspaceController::SetOnProjectStateChanged(std::function<void()> callback)
{
    m_OnProjectStateChanged = std::move(callback);
}

void EditorWorkspaceController::SetOnExitRequested(std::function<void()> callback)
{
    m_OnExitRequested = std::move(callback);
}

void EditorWorkspaceController::SetProjectRoot(const std::filesystem::path& projectRoot)
{
    if (projectRoot.empty()) {
        m_ProjectRoot.clear();
    } else {
        std::error_code error;
        const std::filesystem::path canonicalPath =
            std::filesystem::weakly_canonical(projectRoot, error);
        m_ProjectRoot = error ? projectRoot.lexically_normal() : canonicalPath;
    }

    RebuildProjectView();
    NotifyProjectStateChanged();
}

void EditorWorkspaceController::RefreshProjectTree()
{
    RebuildProjectView();
    NotifyProjectStateChanged();
}

bool EditorWorkspaceController::SelectProjectRoot(ImApplication& app)
{
    FOpenFolderDialogOptions options;
    options.Title = "Select Project Root";
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        return false;
    }

    return RequestProjectRootChange(app, dialogResult.Path);
}

bool EditorWorkspaceController::RequestProjectRootChange(
    ImApplication& app,
    const std::filesystem::path& projectRoot)
{
    if (projectRoot.empty()) {
        return false;
    }

    std::error_code error;
    const std::filesystem::path canonicalTarget = std::filesystem::weakly_canonical(projectRoot, error);
    const std::filesystem::path normalizedTarget =
        error ? projectRoot.lexically_normal() : canonicalTarget;
    if (!m_ProjectRoot.empty()) {
        error.clear();
        const std::filesystem::path currentProjectRoot =
            std::filesystem::weakly_canonical(m_ProjectRoot, error);
        const std::filesystem::path normalizedCurrent =
            error ? m_ProjectRoot.lexically_normal() : currentProjectRoot;
        if (normalizedCurrent == normalizedTarget) {
            return true;
        }
    }

    for (const FDocumentEntry& entry : m_Documents) {
        if (entry.Session &&
            entry.Session->GetDocument() &&
            entry.Session->GetDocument()->IsDirty()) {
            PromptProjectRootChangeWithDirtyDocuments(app, normalizedTarget);
            return false;
        }
    }

    SetProjectRoot(normalizedTarget);
    if (m_OutputText) {
        m_OutputText->SetItems({"Project root: " + normalizedTarget.string()});
    }
    return true;
}

bool EditorWorkspaceController::CreateDocumentInDirectory(ImApplication& app, const std::filesystem::path& directoryPath)
{
    if (directoryPath.empty()) {
        return false;
    }

    OpenCreateDocumentDialog(app, directoryPath);
    return true;
}

bool EditorWorkspaceController::CreateFolderInDirectory(ImApplication& app, const std::filesystem::path& directoryPath)
{
    if (directoryPath.empty()) {
        return false;
    }

    OpenCreateFolderDialog(app, directoryPath);
    return true;
}

void EditorWorkspaceController::Bind(
    const std::shared_ptr<EditorShellHost>& shellHost,
    const std::shared_ptr<ImTabView>& documentTabs,
    const std::shared_ptr<ImTextOutlineView>& projectView,
    const std::shared_ptr<ImTextOutlineView>& widgetTreeView,
    const std::shared_ptr<ReflectionDetailsView>& detailsView,
    const std::shared_ptr<ImTextList>& outputText)
{
    m_ShellHost = shellHost;
    m_DocumentTabs = documentTabs;
    m_ProjectView = projectView;
    m_WidgetTreeView = widgetTreeView;
    m_DetailsView = detailsView;
    m_OutputText = outputText;

    if (m_DocumentTabs) {
        m_DocumentTabs->OnActiveTabChanged.Clear();
        m_DocumentTabs->OnActiveTabChanged.AddLambda(
            [this](ImTabView&, int index) {
                if (!m_bIgnoringTabActivation) {
                    ActivateDocumentTab(index);
                }
            });
        m_DocumentTabs->OnTabCloseRequested.Clear();
        m_DocumentTabs->OnTabCloseRequested.AddLambda(
            [this](ImTabView& tabView, int index, bool& bAllowClose) {
                HandleDocumentTabCloseRequested(tabView, index, bAllowClose);
            });
        m_DocumentTabs->OnTabClosed.Clear();
        m_DocumentTabs->OnTabClosed.AddLambda(
            [this](ImTabView& tabView, int closedIndex) {
                HandleDocumentTabClosed(tabView, closedIndex);
            });
        m_DocumentTabs->OnTabContextMenuRequested.Clear();
        m_DocumentTabs->OnTabContextMenuRequested.AddLambda(
            [this](ImTabView& tabView, int index, FVector2 position) {
                HandleDocumentTabContextMenuRequested(tabView, index, position);
            });
    }

    if (m_ProjectView) {
        m_ProjectView->OnSelectionChanged.Clear();
        m_ProjectView->OnSelectionChanged.AddLambda(
            [this](ImTextOutlineView& view, ImTextOutlineItem* item) {
                HandleProjectSelectionChanged(view, item);
            });
        m_ProjectView->OnItemContextMenuRequested.Clear();
        m_ProjectView->OnItemContextMenuRequested.AddLambda(
            [this](ImTextOutlineView& view, ImTextOutlineItem& item, FVector2 position) {
                HandleProjectItemContextMenuRequested(view, item, position);
            });
    }

    if (m_Documents.empty()) {
        AddSession(CreateSession(), true);
    } else {
        ActivateDocumentTab(m_DocumentTabs ? m_DocumentTabs->GetActiveTabIndex() : 0);
    }

    RebuildProjectView();
}

bool EditorWorkspaceController::NewDocument()
{
    return AddSession(CreateSession(), true);
}

bool EditorWorkspaceController::OpenDocument(ImApplication& app)
{
    std::shared_ptr<EditorSession> session = CreateSession();
    if (!session || !session->OpenDocument(app)) {
        return false;
    }

    const std::shared_ptr<EditorDocument> openedDocument = session->GetDocument();
    if (openedDocument && openedDocument->HasFilePath()) {
        const int existingIndex = FindDocumentIndexByPath(openedDocument->GetFilePath());
        if (existingIndex >= 0) {
            ActivateDocumentAt(existingIndex);
            if (m_OutputText) {
                m_OutputText->SetItems({"Switched to already open document: " + openedDocument->GetFilePath().filename().string()});
            }
            RememberRecentFile(openedDocument->GetFilePath());
            RebuildProjectView();
            return true;
        }

        RememberRecentFile(openedDocument->GetFilePath());
    }

    return AddSession(session, true);
}

bool EditorWorkspaceController::OpenDocumentFromPath(const std::filesystem::path& filePath)
{
    if (filePath.empty()) {
        return false;
    }

    const int existingIndex = FindDocumentIndexByPath(filePath);
    if (existingIndex >= 0) {
        ActivateDocumentAt(existingIndex);
        return true;
    }

    std::shared_ptr<EditorSession> session = CreateSession();
    if (!session || !session->OpenDocumentFromPath(filePath)) {
        return false;
    }

    RememberRecentFile(filePath);
    return AddSession(session, true);
}

bool EditorWorkspaceController::SaveDocument(ImApplication& app)
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->SaveDocument(app) : false;
}

bool EditorWorkspaceController::SaveDocumentAs(ImApplication& app)
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->SaveDocumentAs(app) : false;
}

bool EditorWorkspaceController::GenerateActiveDocumentCpp(ImApplication& app)
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->GenerateCppFiles(app) : false;
}

bool EditorWorkspaceController::CloseActiveDocument(ImApplication& app)
{
    return CloseDocumentAt(app, m_ActiveDocumentIndex);
}

bool EditorWorkspaceController::ActivateDocumentAt(int index)
{
    if (!m_DocumentTabs || index < 0 || index >= static_cast<int>(m_Documents.size())) {
        return false;
    }

    m_bIgnoringTabActivation = true;
    const bool bActivated = m_DocumentTabs->SetActiveTab(index);
    m_bIgnoringTabActivation = false;
    if (!bActivated) {
        return false;
    }

    ActivateDocumentTab(index);
    return true;
}

bool EditorWorkspaceController::ActivateAdjacentDocument(int direction)
{
    if (m_Documents.empty() || direction == 0) {
        return false;
    }

    const int count = static_cast<int>(m_Documents.size());
    const int startIndex = m_ActiveDocumentIndex >= 0 ? m_ActiveDocumentIndex : 0;
    int nextIndex = startIndex + direction;
    if (nextIndex < 0) {
        nextIndex = count - 1;
    } else if (nextIndex >= count) {
        nextIndex = 0;
    }

    return ActivateDocumentAt(nextIndex);
}

bool EditorWorkspaceController::CutSelectedWidget()
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->CutSelectedWidget() : false;
}

bool EditorWorkspaceController::CopySelectedWidget()
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->CopySelectedWidget() : false;
}

bool EditorWorkspaceController::PasteCopiedWidget()
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->PasteCopiedWidget() : false;
}

bool EditorWorkspaceController::DuplicateSelectedWidget()
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->DuplicateSelectedWidget() : false;
}

bool EditorWorkspaceController::Undo()
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->Undo() : false;
}

bool EditorWorkspaceController::Redo()
{
    std::shared_ptr<EditorSession> session = GetActiveSession();
    return session ? session->Redo() : false;
}

bool EditorWorkspaceController::RequestApplicationClose(ImApplication& app)
{
    for (const FDocumentEntry& entry : m_Documents) {
        if (entry.Session &&
            entry.Session->GetDocument() &&
            entry.Session->GetDocument()->IsDirty()) {
            PromptExitWithDirtyDocuments(app);
            return false;
        }
    }

    ConfirmApplicationClose();
    return true;
}

void EditorWorkspaceController::ConfirmApplicationClose()
{
    ClosePendingPrompt();
    m_bExitRequested = false;
    m_ExitPromptAppLock.reset();
    if (m_OnExitRequested) {
        m_OnExitRequested();
    }
}

bool EditorWorkspaceController::SaveWorkspaceState(const std::filesystem::path& filePath) const
{
    if (filePath.empty()) {
        return false;
    }

    try {
        const std::filesystem::path parentPath = filePath.parent_path();
        if (!parentPath.empty()) {
            std::error_code error;
            std::filesystem::create_directories(parentPath, error);
        }

        std::ofstream stream(filePath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            return false;
        }

        stream << BuildWorkspaceStateJson().dump(2);
        stream.flush();
        return stream.good();
    } catch (...) {
        return false;
    }
}

bool EditorWorkspaceController::LoadWorkspaceState(const std::filesystem::path& filePath)
{
    if (filePath.empty() || !std::filesystem::exists(filePath)) {
        return false;
    }

    try {
        std::ifstream stream(filePath, std::ios::binary);
        if (!stream.is_open()) {
            return false;
        }

        json workspaceState;
        stream >> workspaceState;
        return ApplyWorkspaceStateJson(workspaceState, nullptr);
    } catch (...) {
        return false;
    }
}

std::shared_ptr<EditorSession> EditorWorkspaceController::GetActiveSession() const
{
    if (m_ActiveDocumentIndex < 0 ||
        m_ActiveDocumentIndex >= static_cast<int>(m_Documents.size())) {
        return nullptr;
    }

    return m_Documents[static_cast<std::size_t>(m_ActiveDocumentIndex)].Session;
}

std::shared_ptr<EditorSession> EditorWorkspaceController::CreateSession() const
{
    return std::make_shared<EditorSession>(m_CreateDefaultDocumentRoot);
}

EditorWorkspaceController::FSessionWidgets EditorWorkspaceController::CreateSessionWidgets() const
{
    FSessionWidgets widgets;

    widgets.DocumentHost = CreateDocumentHost();
    widgets.PreviewHost = CreatePreviewHost();
    widgets.SchemaText = CreateSchemaText();
    widgets.HeaderPreviewText = CreateCodePreviewText();
    widgets.SourcePreviewText = CreateCodePreviewText();
    widgets.DesignerSurface = std::make_shared<ImDesignerSurface>();
    widgets.DocumentHost->SetContent(widgets.DesignerSurface);

    widgets.WorkspaceTabs = std::make_shared<ImTabView>();
    widgets.WorkspaceTabs->SetSupportsKeyboardFocus(true);
    widgets.WorkspaceTabs->SetStyle(CreateWorkspaceTabStyle());
    widgets.WorkspaceTabs->AddTab("Designer", widgets.DocumentHost);
    widgets.WorkspaceTabs->AddTab("Preview", widgets.PreviewHost);
    widgets.WorkspaceTabs->AddTab("Schema", widgets.SchemaText);
    widgets.WorkspaceTabs->AddTab(".h", widgets.HeaderPreviewText);
    widgets.WorkspaceTabs->AddTab(".cpp", widgets.SourcePreviewText);

    widgets.Root = widgets.WorkspaceTabs;
    return widgets;
}

bool EditorWorkspaceController::AddSession(const std::shared_ptr<EditorSession>& session, bool bActivateNewTab)
{
    if (!session || !m_DocumentTabs) {
        return false;
    }

    FSessionWidgets widgets = CreateSessionWidgets();
    const std::string tabTitle = session->GetDocumentTabTitle();

    m_bIgnoringTabActivation = true;
    const int tabIndex = m_DocumentTabs->AddTab(tabTitle, widgets.Root);
    m_bIgnoringTabActivation = false;
    if (tabIndex < 0) {
        return false;
    }

    m_DocumentTabs->SetTabClosable(tabIndex, true);
    session->SetDocumentTabBinding(m_DocumentTabs, tabIndex);
    m_Documents.push_back(FDocumentEntry {session, std::move(widgets)});

    if (bActivateNewTab) {
        m_bIgnoringTabActivation = true;
        m_DocumentTabs->SetActiveTab(tabIndex);
        m_bIgnoringTabActivation = false;
        ActivateDocumentTab(tabIndex);
    } else if (m_ActiveDocumentIndex < 0) {
        ActivateDocumentTab(tabIndex);
    }

    RebuildProjectView();
    return true;
}

void EditorWorkspaceController::ActivateDocumentTab(int index)
{
    if (index < 0 || index >= static_cast<int>(m_Documents.size())) {
        return;
    }

    m_ActiveDocumentIndex = index;
    FDocumentEntry& entry = m_Documents[static_cast<std::size_t>(index)];
    if (m_ShellHost) {
        m_ShellHost->SetSession(entry.Session);
    }

    entry.Session->BindDocumentWidgets(
        m_DocumentTabs,
        index,
        entry.Widgets.DocumentHost,
        entry.Widgets.PreviewHost,
        entry.Widgets.SchemaText,
        entry.Widgets.DesignerSurface,
        m_WidgetTreeView,
        m_DetailsView,
        m_OutputText,
        entry.Widgets.HeaderPreviewText,
        entry.Widgets.SourcePreviewText);

    RebuildProjectView();
}

bool EditorWorkspaceController::CloseDocumentAt(ImApplication& app, int index)
{
    if (index < 0 || index >= static_cast<int>(m_Documents.size()) || !m_DocumentTabs) {
        return false;
    }

    FDocumentEntry& entry = m_Documents[static_cast<std::size_t>(index)];
    if (entry.Session &&
        entry.Session->GetDocument() &&
        entry.Session->GetDocument()->IsDirty()) {
        PromptCloseDirtyDocument(app, index);
        return false;
    }

    return FinalizeDocumentClose(index);
}

void EditorWorkspaceController::HandleDocumentTabCloseRequested(ImTabView&, int index, bool& bAllowClose)
{
    bAllowClose = false;

    if (!m_ShellHost) {
        return;
    }

    ImApplication* application = m_ShellHost->GetApplication();
    if (application == nullptr) {
        return;
    }

    if (m_PendingCloseDocumentIndex >= 0) {
        ClosePendingPrompt();
    }

    if (index < 0 || index >= static_cast<int>(m_Documents.size())) {
        return;
    }

    FDocumentEntry& entry = m_Documents[static_cast<std::size_t>(index)];
    if (entry.Session &&
        entry.Session->GetDocument() &&
        entry.Session->GetDocument()->IsDirty()) {
        PromptCloseDirtyDocument(*application, index);
        return;
    }

    bAllowClose = true;
}

void EditorWorkspaceController::HandleDocumentTabClosed(ImTabView&, int closedIndex)
{
    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();

    if (closedIndex < 0 || closedIndex >= static_cast<int>(m_Documents.size())) {
        return;
    }

    m_Documents.erase(m_Documents.begin() + closedIndex);
    for (int index = closedIndex; index < static_cast<int>(m_Documents.size()); ++index) {
        m_Documents[static_cast<std::size_t>(index)].Session->SetDocumentTabBinding(m_DocumentTabs, index);
    }

    if (m_Documents.empty()) {
        m_ActiveDocumentIndex = -1;
        if (m_ShellHost) {
            m_ShellHost->SetSession(nullptr);
        }
        if (m_ProjectView) {
            m_ProjectView->ClearItems();
        }
        if (m_WidgetTreeView) {
            m_WidgetTreeView->ClearItems();
        }
        if (m_DetailsView) {
            m_DetailsView->SetTargets(nullptr, nullptr);
        }
        if (m_OutputText) {
            m_OutputText->SetItems({"No open documents."});
        }
        return;
    }

    const int nextIndex = std::clamp(
        m_DocumentTabs ? m_DocumentTabs->GetActiveTabIndex() : closedIndex,
        0,
        static_cast<int>(m_Documents.size()) - 1);
    ActivateDocumentTab(nextIndex);
}

void EditorWorkspaceController::HandleDocumentTabContextMenuRequested(ImTabView&, int index, FVector2 position)
{
    if (!m_ShellHost) {
        return;
    }

    ImApplication* application = m_ShellHost->GetApplication();
    if (application == nullptr) {
        return;
    }

    OpenDocumentTabContextMenu(*application, index, position);
}

void EditorWorkspaceController::HandleProjectItemContextMenuRequested(
    ImTextOutlineView&,
    ImTextOutlineItem& item,
    FVector2 position)
{
    if (!m_ShellHost) {
        return;
    }

    ImApplication* application = m_ShellHost->GetApplication();
    if (application == nullptr) {
        return;
    }

    OpenProjectItemContextMenu(*application, &item, position);
}

bool EditorWorkspaceController::FinalizeDocumentClose(int index)
{
    if (!m_DocumentTabs || index < 0 || index >= static_cast<int>(m_Documents.size())) {
        return false;
    }

    ClosePendingPrompt();

    m_bIgnoringTabActivation = true;
    const bool bRemoved = m_DocumentTabs->RemoveTab(index);
    m_bIgnoringTabActivation = false;
    return bRemoved;
}

bool EditorWorkspaceController::CloseOtherDocuments(ImApplication& app, int keepIndex)
{
    if (keepIndex < 0 || keepIndex >= static_cast<int>(m_Documents.size())) {
        return false;
    }

    bool bClosedAny = false;
    for (int index = static_cast<int>(m_Documents.size()) - 1; index >= 0; --index) {
        if (index == keepIndex) {
            continue;
        }

        FDocumentEntry& entry = m_Documents[static_cast<std::size_t>(index)];
        if (entry.Session &&
            entry.Session->GetDocument() &&
            entry.Session->GetDocument()->IsDirty()) {
            ActivateDocumentAt(index);
            PromptCloseDirtyDocument(app, index);
            return bClosedAny;
        }

        bClosedAny = FinalizeDocumentClose(index) || bClosedAny;
        if (index < keepIndex) {
            --keepIndex;
        }
    }

    ActivateDocumentAt(keepIndex);
    return bClosedAny;
}

bool EditorWorkspaceController::CloseAllDocuments(ImApplication& app)
{
    bool bClosedAny = false;
    for (int index = static_cast<int>(m_Documents.size()) - 1; index >= 0; --index) {
        FDocumentEntry& entry = m_Documents[static_cast<std::size_t>(index)];
        if (entry.Session &&
            entry.Session->GetDocument() &&
            entry.Session->GetDocument()->IsDirty()) {
            ActivateDocumentAt(index);
            PromptCloseDirtyDocument(app, index);
            return bClosedAny;
        }

        bClosedAny = FinalizeDocumentClose(index) || bClosedAny;
    }

    return bClosedAny;
}

void EditorWorkspaceController::PromptCloseDirtyDocument(ImApplication& app, int index)
{
    if (index < 0 || index >= static_cast<int>(m_Documents.size())) {
        return;
    }

    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    m_PendingCloseDocumentIndex = index;

    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle popupStyle = popupMenu->GetStyle();
    popupStyle.CornerRadius = 6.0f;
    popupMenu->SetStyle(popupStyle);

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {
        "Save and Close",
        {},
        {},
        true,
        false,
        [weakThis, application = &app]() {
            if (auto self = weakThis.lock()) {
                const int pendingIndex = self->m_PendingCloseDocumentIndex;
                if (pendingIndex < 0 || pendingIndex >= static_cast<int>(self->m_Documents.size())) {
                    self->ClosePendingPrompt();
                    return;
                }

                auto session = self->m_Documents[static_cast<std::size_t>(pendingIndex)].Session;
                const bool bSaved = session && application && session->SaveDocument(*application);
                if (bSaved) {
                    self->FinalizeDocumentClose(pendingIndex);
                }
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Discard Changes",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->FinalizeDocumentClose(self->m_PendingCloseDocumentIndex);
            }
        }
    });
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Cancel",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->ClosePendingPrompt();
            }
        }
    });
    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu&, int) {
        if (auto self = weakThis.lock()) {
            self->ClosePendingPrompt();
        }
    });

    FVector2 popupPosition(160.0f, 96.0f);
    if (m_DocumentTabs) {
        const FGeometry geometry = m_DocumentTabs->GetGeometry();
        popupPosition = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.5f - 100.0f),
            geometry.Position.Y + 44.0f);
    }

    FPopupOptions popupOptions;
    popupOptions.Title = "CloseDirtyDocumentPrompt";
    popupOptions.Position = popupPosition;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_CloseConfirmMenu = popupMenu;
    m_CloseConfirmWindow = app.GetWindowManager().CreatePopup(popupOptions);
}

void EditorWorkspaceController::PromptExitWithDirtyDocuments(ImApplication& app)
{
    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();
    m_bExitRequested = true;
    m_ExitPromptAppLock = std::shared_ptr<ImApplication>(&app, [](ImApplication*) {});

    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle popupStyle = popupMenu->GetStyle();
    popupStyle.CornerRadius = 6.0f;
    popupMenu->SetStyle(popupStyle);

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {
        "Save All and Exit",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                std::shared_ptr<ImApplication> app = self->m_ExitPromptAppLock;
                if (!app) {
                    self->ClosePendingPrompt();
                    return;
                }

                for (int index = 0; index < static_cast<int>(self->m_Documents.size()); ++index) {
                    self->ActivateDocumentAt(index);
                    auto session = self->m_Documents[static_cast<std::size_t>(index)].Session;
                    if (!session || !session->GetDocument() || !session->GetDocument()->IsDirty()) {
                        continue;
                    }

                    if (!session->SaveDocument(*app)) {
                        self->m_bExitRequested = false;
                        self->m_ExitPromptAppLock.reset();
                        return;
                    }
                }

                self->ConfirmApplicationClose();
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Discard Changes and Exit",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->ConfirmApplicationClose();
            }
        }
    });
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Cancel",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->m_bExitRequested = false;
                self->m_ExitPromptAppLock.reset();
                self->ClosePendingPrompt();
            }
        }
    });
    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu&, int) {
        if (auto self = weakThis.lock()) {
            if (!self->m_bExitRequested) {
                self->ClosePendingPrompt();
            }
        }
    });

    FVector2 popupPosition(160.0f, 96.0f);
    if (m_DocumentTabs) {
        const FGeometry geometry = m_DocumentTabs->GetGeometry();
        popupPosition = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.5f - 120.0f),
            geometry.Position.Y + 44.0f);
    }

    FPopupOptions popupOptions;
    popupOptions.Title = "ExitDirtyDocumentsPrompt";
    popupOptions.Position = popupPosition;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_CloseConfirmMenu = popupMenu;
    m_CloseConfirmWindow = app.GetWindowManager().CreatePopup(popupOptions);
}

void EditorWorkspaceController::PromptProjectRootChangeWithDirtyDocuments(
    ImApplication& app,
    const std::filesystem::path& projectRoot)
{
    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();
    m_PendingProjectRootChange = projectRoot;

    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle popupStyle = popupMenu->GetStyle();
    popupStyle.CornerRadius = 6.0f;
    popupMenu->SetStyle(popupStyle);

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {
        "Save All and Switch",
        {},
        {},
        true,
        false,
        [weakThis, application = &app]() {
            if (auto self = weakThis.lock()) {
                for (int index = 0; index < static_cast<int>(self->m_Documents.size()); ++index) {
                    self->ActivateDocumentAt(index);
                    auto session = self->m_Documents[static_cast<std::size_t>(index)].Session;
                    if (!session || !session->GetDocument() || !session->GetDocument()->IsDirty()) {
                        continue;
                    }

                    if (!application || !session->SaveDocument(*application)) {
                        self->m_PendingProjectRootChange.clear();
                        return;
                    }
                }

                const std::filesystem::path pendingProjectRoot = self->m_PendingProjectRootChange;
                self->m_PendingProjectRootChange.clear();
                self->SetProjectRoot(pendingProjectRoot);
                if (self->m_OutputText) {
                    self->m_OutputText->SetItems({"Project root: " + pendingProjectRoot.string()});
                }
                self->ClosePendingPrompt();
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Discard Changes and Switch",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                const std::filesystem::path pendingProjectRoot = self->m_PendingProjectRootChange;
                self->m_PendingProjectRootChange.clear();
                self->SetProjectRoot(pendingProjectRoot);
                if (self->m_OutputText) {
                    self->m_OutputText->SetItems({"Project root: " + pendingProjectRoot.string()});
                }
                self->ClosePendingPrompt();
            }
        }
    });
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Cancel",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->m_PendingProjectRootChange.clear();
                self->ClosePendingPrompt();
            }
        }
    });
    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu&, int) {
        if (auto self = weakThis.lock()) {
            if (self->m_PendingProjectRootChange.empty()) {
                self->ClosePendingPrompt();
            }
        }
    });

    FVector2 popupPosition(160.0f, 96.0f);
    if (m_DocumentTabs) {
        const FGeometry geometry = m_DocumentTabs->GetGeometry();
        popupPosition = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.5f - 120.0f),
            geometry.Position.Y + 44.0f);
    }

    FPopupOptions popupOptions;
    popupOptions.Title = "ProjectRootChangeDirtyDocumentsPrompt";
    popupOptions.Position = popupPosition;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_CloseConfirmMenu = popupMenu;
    m_CloseConfirmWindow = app.GetWindowManager().CreatePopup(popupOptions);
}

void EditorWorkspaceController::OpenRenameProjectItemDialog(
    ImApplication& app,
    const std::filesystem::path& path)
{
    if (path.empty() || !std::filesystem::exists(path)) {
        return;
    }

    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();
    m_PendingRenameProjectItemPath = path;

    auto weakThis = weak_from_this();
    FInputDialogOptions dialogOptions;
    dialogOptions.PopupTitle = "RenameProjectItemDialog";
    dialogOptions.HeadingText = std::filesystem::is_directory(path) ? "Rename Folder" : "Rename File";
    dialogOptions.InitialText = path.filename().string();
    dialogOptions.ConfirmText = "Rename";
    dialogOptions.CancelText = "Cancel";
    dialogOptions.Size = FVector2(360.0f, 116.0f);
    dialogOptions.bSelectAllOnOpen = true;
    dialogOptions.OnConfirm = [weakThis](const std::string& newName) {
        if (auto self = weakThis.lock()) {
            const std::filesystem::path oldPath = self->m_PendingRenameProjectItemPath;
            if (self->RenameProjectItem(oldPath, newName)) {
                self->m_PendingRenameProjectItemPath.clear();
                return true;
            }
        }
        return false;
    };
    dialogOptions.OnCancel = [weakThis]() {
        if (auto self = weakThis.lock()) {
            self->m_PendingRenameProjectItemPath.clear();
        }
    };
    dialogOptions.Position = FVector2(220.0f, 120.0f);
    if (m_ProjectView) {
        const FGeometry geometry = m_ProjectView->GetGeometry();
        dialogOptions.Position = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.30f),
            geometry.Position.Y + 52.0f);
    }

    m_PendingInputDialog = std::make_shared<InputDialog>();
    m_PendingInputDialog->Open(app, dialogOptions);
    m_CloseConfirmWindow = m_PendingInputDialog->GetWindow();
    m_CloseConfirmMenu.reset();
}

void EditorWorkspaceController::OpenCreateDocumentDialog(
    ImApplication& app,
    const std::filesystem::path& directoryPath)
{
    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();

    auto weakThis = weak_from_this();
    const std::filesystem::path defaultDocumentPath =
        BuildUniqueChildFilePath(directoryPath, "NewWidget", ".ui.json");
    FInputDialogOptions dialogOptions;
    dialogOptions.PopupTitle = "CreateDocumentDialog";
    dialogOptions.HeadingText = "Create UI Document";
    dialogOptions.InitialText = defaultDocumentPath.empty()
        ? std::string("NewWidget.ui.json")
        : defaultDocumentPath.filename().string();
    dialogOptions.ConfirmText = "Create";
    dialogOptions.CancelText = "Cancel";
    dialogOptions.Size = FVector2(400.0f, 116.0f);
    dialogOptions.bSelectAllOnOpen = true;
    dialogOptions.OnConfirm = [weakThis, directoryPath](const std::string& fileName) {
        if (auto self = weakThis.lock()) {
            const std::filesystem::path trimmedName = std::filesystem::path(fileName).filename();
            if (trimmedName.empty() || trimmedName != fileName) {
                if (self->m_OutputText) {
                    self->m_OutputText->SetItems({"Create document failed: file name must not contain path separators."});
                }
                return false;
            }

            std::string normalizedName = trimmedName.string();
            if (!EndsWithCaseInsensitive(normalizedName, ".json")) {
                normalizedName += ".ui.json";
            }

            return self->CreateAndOpenDocumentAtPath(directoryPath / normalizedName);
        }
        return false;
    };
    dialogOptions.Position = FVector2(220.0f, 120.0f);
    if (m_ProjectView) {
        const FGeometry geometry = m_ProjectView->GetGeometry();
        dialogOptions.Position = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.30f),
            geometry.Position.Y + 52.0f);
    }

    m_PendingInputDialog = std::make_shared<InputDialog>();
    m_PendingInputDialog->Open(app, dialogOptions);
    m_CloseConfirmWindow = m_PendingInputDialog->GetWindow();
    m_CloseConfirmMenu.reset();
}

void EditorWorkspaceController::OpenCreateFolderDialog(
    ImApplication& app,
    const std::filesystem::path& directoryPath)
{
    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();

    auto weakThis = weak_from_this();
    const std::filesystem::path defaultFolderPath = BuildUniqueChildPath(directoryPath, "NewFolder");
    FInputDialogOptions dialogOptions;
    dialogOptions.PopupTitle = "CreateFolderDialog";
    dialogOptions.HeadingText = "Create Folder";
    dialogOptions.InitialText = defaultFolderPath.empty()
        ? std::string("NewFolder")
        : defaultFolderPath.filename().string();
    dialogOptions.ConfirmText = "Create";
    dialogOptions.CancelText = "Cancel";
    dialogOptions.Size = FVector2(360.0f, 116.0f);
    dialogOptions.bSelectAllOnOpen = true;
    dialogOptions.OnConfirm = [weakThis, directoryPath](const std::string& folderName) {
        if (auto self = weakThis.lock()) {
            const std::filesystem::path trimmedName = std::filesystem::path(folderName).filename();
            if (trimmedName.empty() || trimmedName != folderName) {
                if (self->m_OutputText) {
                    self->m_OutputText->SetItems({"Create folder failed: folder name must not contain path separators."});
                }
                return false;
            }

            return self->CreateFolderAtPath(directoryPath / trimmedName);
        }
        return false;
    };
    dialogOptions.Position = FVector2(220.0f, 120.0f);
    if (m_ProjectView) {
        const FGeometry geometry = m_ProjectView->GetGeometry();
        dialogOptions.Position = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.30f),
            geometry.Position.Y + 52.0f);
    }

    m_PendingInputDialog = std::make_shared<InputDialog>();
    m_PendingInputDialog->Open(app, dialogOptions);
    m_CloseConfirmWindow = m_PendingInputDialog->GetWindow();
    m_CloseConfirmMenu.reset();
}

void EditorWorkspaceController::PromptDeleteProjectItem(
    ImApplication& app,
    const std::filesystem::path& path)
{
    if (path.empty()) {
        return;
    }

    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();
    m_PendingDeleteProjectItemPath = path;

    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle popupStyle = popupMenu->GetStyle();
    popupStyle.CornerRadius = 6.0f;
    popupMenu->SetStyle(popupStyle);

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {
        std::filesystem::is_directory(path) ? "Delete Folder" : "Delete File",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                const std::filesystem::path pendingPath = self->m_PendingDeleteProjectItemPath;
                self->m_PendingDeleteProjectItemPath.clear();
                self->DeleteProjectItem(pendingPath);
                self->ClosePendingPrompt();
            }
        }
    });
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Cancel",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->m_PendingDeleteProjectItemPath.clear();
                self->ClosePendingPrompt();
            }
        }
    });
    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu&, int) {
        if (auto self = weakThis.lock()) {
            if (self->m_PendingDeleteProjectItemPath.empty()) {
                self->ClosePendingPrompt();
            }
        }
    });

    FVector2 popupPosition(160.0f, 96.0f);
    if (m_ProjectView) {
        const FGeometry geometry = m_ProjectView->GetGeometry();
        popupPosition = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.35f),
            geometry.Position.Y + 48.0f);
    }

    FPopupOptions popupOptions;
    popupOptions.Title = "DeleteProjectItemPrompt";
    popupOptions.Position = popupPosition;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_CloseConfirmMenu = popupMenu;
    m_CloseConfirmWindow = app.GetWindowManager().CreatePopup(popupOptions);
}

void EditorWorkspaceController::ClosePendingPrompt()
{
    if (m_CloseConfirmWindow && m_ShellHost && m_ShellHost->GetApplication()) {
        m_ShellHost->GetApplication()->GetWindowManager().CloseWindow(m_CloseConfirmWindow);
    }

    m_CloseConfirmMenu.reset();
    m_CloseConfirmWindow.reset();
    m_PendingInputDialog.reset();
    m_PendingCloseDocumentIndex = -1;
    m_PendingProjectRootChange.clear();
    m_PendingRenameProjectItemPath.clear();
    m_PendingDeleteProjectItemPath.clear();
}

EditorWorkspaceController::json EditorWorkspaceController::BuildWorkspaceStateJson() const
{
    json workspaceState;
    workspaceState["Format"] = "ImWidgetV4EditorWorkspaceState";
    workspaceState["Version"] = 1;
    workspaceState["ProjectRoot"] = m_ProjectRoot.string();
    workspaceState["ActiveDocumentIndex"] = m_ActiveDocumentIndex;

    json openDocuments = json::array();
    for (const FDocumentEntry& entry : m_Documents) {
        if (!entry.Session || !entry.Session->GetDocument()) {
            continue;
        }

        const std::shared_ptr<EditorDocument>& document = entry.Session->GetDocument();
        if (!document->HasFilePath()) {
            continue;
        }

        json documentJson;
        documentJson["Path"] = document->GetFilePath().string();
        openDocuments.push_back(std::move(documentJson));
    }
    workspaceState["OpenDocuments"] = std::move(openDocuments);

    json recentFiles = json::array();
    for (const std::filesystem::path& path : m_RecentFiles) {
        recentFiles.push_back(path.string());
    }
    workspaceState["RecentFiles"] = std::move(recentFiles);

    return workspaceState;
}

bool EditorWorkspaceController::ApplyWorkspaceStateJson(const json& workspaceState, std::string* outError)
{
    if (!workspaceState.is_object()) {
        if (outError) {
            *outError = "Workspace state must be an object.";
        }
        return false;
    }

    if (workspaceState.value("Format", std::string()) != "ImWidgetV4EditorWorkspaceState") {
        if (outError) {
            *outError = "Unsupported workspace state format.";
        }
        return false;
    }

    const int version = workspaceState.value("Version", 0);
    if (version != 1) {
        if (outError) {
            *outError = "Unsupported workspace state version.";
        }
        return false;
    }

    const std::string projectRoot = workspaceState.value("ProjectRoot", std::string());
    if (!projectRoot.empty()) {
        SetProjectRoot(projectRoot);
    }

    if (workspaceState.contains("RecentFiles") && workspaceState["RecentFiles"].is_array()) {
        m_RecentFiles.clear();
        for (const json& recentFile : workspaceState["RecentFiles"]) {
            if (recentFile.is_string()) {
                RememberRecentFile(recentFile.get<std::string>());
            }
        }
    }

    if (m_DocumentTabs) {
        m_DocumentTabs->ClearTabs();
    }
    m_Documents.clear();
    m_ActiveDocumentIndex = -1;
    if (m_ShellHost) {
        m_ShellHost->SetSession(nullptr);
    }

    bool bAddedAnyDocument = false;
    if (workspaceState.contains("OpenDocuments") && workspaceState["OpenDocuments"].is_array()) {
        for (const json& documentEntry : workspaceState["OpenDocuments"]) {
            if (!documentEntry.is_object()) {
                continue;
            }

            const std::filesystem::path filePath = documentEntry.value("Path", std::string());
            if (filePath.empty() || !std::filesystem::exists(filePath)) {
                continue;
            }

            std::shared_ptr<EditorSession> session = CreateSession();
            if (!session || !session->OpenDocumentFromPath(filePath)) {
                continue;
            }

            RememberRecentFile(filePath);
            bAddedAnyDocument = AddSession(session, false) || bAddedAnyDocument;
        }
    }

    if (!bAddedAnyDocument) {
        AddSession(CreateSession(), true);
    } else {
        const int desiredActiveIndex = workspaceState.value("ActiveDocumentIndex", 0);
        ActivateDocumentAt(std::clamp(desiredActiveIndex, 0, static_cast<int>(m_Documents.size()) - 1));
    }

    RebuildProjectView();
    return true;
}

void EditorWorkspaceController::OpenDocumentTabContextMenu(ImApplication& app, int index, FVector2 position)
{
    if (index < 0 || index >= static_cast<int>(m_Documents.size())) {
        return;
    }

    CloseDocumentTabContextMenu();
    m_ContextMenuDocumentIndex = index;

    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle popupStyle = popupMenu->GetStyle();
    popupStyle.CornerRadius = 6.0f;
    popupMenu->SetStyle(popupStyle);

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(FPopupMenuItem {
        "Activate",
        {},
        {},
        true,
        false,
        [weakThis, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                self->CloseDocumentTabContextMenu();
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Save",
        {},
        {},
        true,
        false,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->SaveDocument(*application);
                }
                self->CloseDocumentTabContextMenu();
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Save As...",
        {},
        {},
        true,
        false,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->SaveDocumentAs(*application);
                }
                self->CloseDocumentTabContextMenu();
            }
        }
    });
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Close",
        {},
        {},
        true,
        false,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->CloseDocumentAt(*application, index);
                }
                self->CloseDocumentTabContextMenu();
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Close Others",
        {},
        {},
        true,
        false,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->CloseOtherDocuments(*application, index);
                }
                self->CloseDocumentTabContextMenu();
            }
        }
    });
    items.push_back(FPopupMenuItem {
        "Close All",
        {},
        {},
        true,
        false,
        [weakThis, application = &app]() {
            if (auto self = weakThis.lock()) {
                if (application) {
                    self->CloseAllDocuments(*application);
                }
                self->CloseDocumentTabContextMenu();
            }
        }
    });
    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu&, int) {
        if (auto self = weakThis.lock()) {
            self->CloseDocumentTabContextMenu();
        }
    });

    FPopupOptions popupOptions;
    popupOptions.Title = "DocumentTabContextMenu";
    popupOptions.Position = position;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_DocumentTabContextMenu = popupMenu;
    m_DocumentTabContextMenuWindow = app.GetWindowManager().CreatePopup(popupOptions);
}

void EditorWorkspaceController::CloseDocumentTabContextMenu()
{
    if (m_DocumentTabContextMenuWindow && m_ShellHost && m_ShellHost->GetApplication()) {
        m_ShellHost->GetApplication()->GetWindowManager().CloseWindow(m_DocumentTabContextMenuWindow);
    }

    m_DocumentTabContextMenu.reset();
    m_DocumentTabContextMenuWindow.reset();
    m_ContextMenuDocumentIndex = -1;
}

void EditorWorkspaceController::OpenProjectItemContextMenu(
    ImApplication& app,
    ImTextOutlineItem* item,
    FVector2 position)
{
    if (item == nullptr) {
        return;
    }

    const auto bindingIt = m_ProjectItemBindings.find(item);
    if (bindingIt == m_ProjectItemBindings.end()) {
        return;
    }

    CloseProjectItemContextMenu();
    m_ContextMenuProjectItem = item;

    const FProjectItemBinding binding = bindingIt->second;
    auto popupMenu = std::make_shared<ImPopupMenu>();
    FPopupMenuStyle popupStyle = popupMenu->GetStyle();
    popupStyle.CornerRadius = 6.0f;
    popupMenu->SetStyle(popupStyle);

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;

    if (binding.Kind == EProjectItemKind::OpenDocument) {
        items.push_back(FPopupMenuItem {
            "Activate",
            {},
            {},
            true,
            false,
            [weakThis, index = binding.Index]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    self->CloseProjectItemContextMenu();
                }
            }
        });

        items.push_back(FPopupMenuItem {
            "Save",
            {},
            {},
            true,
            false,
            [weakThis, index = binding.Index, application = &app]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    if (application) {
                        self->SaveDocument(*application);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }
        });

        items.push_back(FPopupMenuItem {
            "Save As...",
            {},
            {},
            true,
            false,
            [weakThis, index = binding.Index, application = &app]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    if (application) {
                        self->SaveDocumentAs(*application);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }
        });

        items.push_back(FPopupMenuItem {
            "Close",
            {},
            {},
            true,
            false,
            [weakThis, index = binding.Index, application = &app]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    if (application) {
                        self->CloseDocumentAt(*application, index);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }
        });
    }

    if (binding.Kind == EProjectItemKind::RecentFile ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        items.push_back(FPopupMenuItem {
            "Open",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path]() {
                if (auto self = weakThis.lock()) {
                    self->OpenDocumentFromPath(path);
                    self->CloseProjectItemContextMenu();
                }
            }
        });
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        items.push_back(FPopupMenuItem {
            "Set As Project Root",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path, kind = binding.Kind, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->RequestProjectRootChange(
                            *application,
                            kind == EProjectItemKind::WorkspaceDirectory ? path : path.parent_path());
                    }
                    self->CloseProjectItemContextMenu();
                }
            }
        });
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory) {
        items.push_back(FPopupMenuItem {
            "New UI Document...",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->CreateDocumentInDirectory(*application, path);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }
        });

        items.push_back(FPopupMenuItem {
            "New Folder",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->CreateFolderInDirectory(*application, path);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }
        });

        items.push_back(FPopupMenuItem {
            "Refresh This Folder",
            {},
            {},
            true,
            false,
            [weakThis]() {
                if (auto self = weakThis.lock()) {
                    self->RefreshProjectTree();
                    self->CloseProjectItemContextMenu();
                }
            }
        });
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
        items.push_back(FPopupMenuItem {
            "Rename...",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->OpenRenameProjectItemDialog(*application, path);
                    }
                }
            }
        });
        items.push_back(FPopupMenuItem {
            "Reveal in Explorer",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path]() {
                if (auto self = weakThis.lock()) {
                    self->RevealProjectItemInExplorer(path);
                    self->CloseProjectItemContextMenu();
                }
            }
        });
        items.push_back(FPopupMenuItem {
            "Delete",
            {},
            {},
            true,
            false,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->PromptDeleteProjectItem(*application, path);
                    }
                }
            }
        });
    }

    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FPopupMenuItem {
        "Refresh Project Tree",
        {},
        {},
        true,
        false,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->RefreshProjectTree();
                self->CloseProjectItemContextMenu();
            }
        }
    });

    popupMenu->SetItems(std::move(items));
    popupMenu->OnItemInvoked.AddLambda([weakThis](ImPopupMenu&, int) {
        if (auto self = weakThis.lock()) {
            self->CloseProjectItemContextMenu();
        }
    });

    FPopupOptions popupOptions;
    popupOptions.Title = "ProjectItemContextMenu";
    popupOptions.Position = position;
    popupOptions.Size = popupMenu->GetMinSize();
    popupOptions.RootWidget = popupMenu;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_ProjectItemContextMenu = popupMenu;
    m_ProjectItemContextMenuWindow = app.GetWindowManager().CreatePopup(popupOptions);
}

void EditorWorkspaceController::CloseProjectItemContextMenu()
{
    if (m_ProjectItemContextMenuWindow && m_ShellHost && m_ShellHost->GetApplication()) {
        m_ShellHost->GetApplication()->GetWindowManager().CloseWindow(m_ProjectItemContextMenuWindow);
    }

    m_ProjectItemContextMenu.reset();
    m_ProjectItemContextMenuWindow.reset();
    m_ContextMenuProjectItem = nullptr;
}

bool EditorWorkspaceController::CreateAndOpenDocumentAtPath(const std::filesystem::path& filePath)
{
    if (filePath.empty()) {
        return false;
    }

    const std::filesystem::path normalizedPath = filePath.lexically_normal();
    const int existingIndex = FindDocumentIndexByPath(normalizedPath);
    if (existingIndex >= 0) {
        ActivateDocumentAt(existingIndex);
        return true;
    }

    std::shared_ptr<EditorSession> session = CreateSession();
    if (!session) {
        return false;
    }

    std::shared_ptr<EditorDocument> document = session->GetDocument();
    if (!document) {
        return false;
    }

    document->SetDisplayTitle(normalizedPath.stem().string());
    std::string errorMessage;
    if (!document->SaveAs(normalizedPath, &errorMessage)) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create document failed: " + errorMessage});
        }
        return false;
    }

    RememberRecentFile(normalizedPath);
    const bool bAdded = AddSession(session, true);
    if (bAdded && m_OutputText) {
        m_OutputText->SetItems({"Created " + normalizedPath.filename().string()});
    }
    return bAdded;
}

bool EditorWorkspaceController::CreateFolderAtPath(const std::filesystem::path& directoryPath)
{
    if (directoryPath.empty()) {
        return false;
    }

    try {
        if (std::filesystem::exists(directoryPath)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create folder skipped: " + directoryPath.filename().string() + " already exists."});
            }
            return false;
        }

        const std::filesystem::path parentPath = directoryPath.parent_path();
        if (!parentPath.empty()) {
            std::filesystem::create_directories(parentPath);
        }

        if (!std::filesystem::create_directory(directoryPath)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create folder failed: unable to create " + directoryPath.filename().string()});
            }
            return false;
        }
    } catch (const std::exception& exception) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create folder failed: " + std::string(exception.what())});
        }
        return false;
    } catch (...) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create folder failed."});
        }
        return false;
    }

    RefreshProjectTree();
    if (m_OutputText) {
        m_OutputText->SetItems({"Created folder " + directoryPath.filename().string()});
    }
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::RenameProjectItem(const std::filesystem::path& path, const std::string& newName)
{
    if (path.empty() || newName.empty() || !std::filesystem::exists(path)) {
        return false;
    }

    const std::filesystem::path trimmedName = std::filesystem::path(newName).filename();
    if (trimmedName.empty() || trimmedName != newName) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Rename failed: name must not contain path separators."});
        }
        return false;
    }

    const std::filesystem::path targetPath = path.parent_path() / trimmedName;
    if (AreEquivalentPaths(targetPath, path)) {
        return true;
    }

    if (std::filesystem::exists(targetPath)) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Rename failed: target already exists."});
        }
        return false;
    }

    for (const FDocumentEntry& entry : m_Documents) {
        if (!entry.Session || !entry.Session->GetDocument() || !entry.Session->GetDocument()->HasFilePath()) {
            continue;
        }

        const std::filesystem::path documentPath = entry.Session->GetDocument()->GetFilePath();
        if ((AreEquivalentPaths(documentPath, path) || IsPathWithinRoot(documentPath, path)) &&
            entry.Session->GetDocument()->IsDirty()) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Rename blocked: save or discard dirty documents under the selected path first."});
            }
            return false;
        }
    }

    try {
        std::filesystem::rename(path, targetPath);
    } catch (const std::exception& exception) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Rename failed: " + std::string(exception.what())});
        }
        return false;
    } catch (...) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Rename failed."});
        }
        return false;
    }

    ReplaceRecentFilePath(path, targetPath);
    UpdateOpenDocumentPathsForRename(path, targetPath);
    if (!m_ProjectRoot.empty() && AreEquivalentPaths(m_ProjectRoot, path)) {
        m_ProjectRoot = targetPath;
    }

    RefreshProjectTree();
    if (m_OutputText) {
        m_OutputText->SetItems({"Renamed " + path.filename().string() + " to " + targetPath.filename().string()});
    }
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::DeleteProjectItem(const std::filesystem::path& path)
{
    if (path.empty() || !std::filesystem::exists(path)) {
        return false;
    }

    bool bBlockedByDirtyDocument = false;
    if (!CloseOpenDocumentsUnderPath(path, &bBlockedByDirtyDocument)) {
        if (bBlockedByDirtyDocument && m_OutputText) {
            m_OutputText->SetItems({"Delete blocked: save or discard dirty documents under the selected path first."});
        }
        return false;
    }

    try {
        RemoveRecentFilesUnderPath(path);
        if (std::filesystem::is_directory(path)) {
            std::filesystem::remove_all(path);
        } else {
            std::filesystem::remove(path);
        }
    } catch (const std::exception& exception) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Delete failed: " + std::string(exception.what())});
        }
        return false;
    } catch (...) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Delete failed."});
        }
        return false;
    }

    RefreshProjectTree();
    if (m_OutputText) {
        m_OutputText->SetItems({"Deleted " + path.filename().string()});
    }
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::RevealProjectItemInExplorer(const std::filesystem::path& path) const
{
    if (path.empty() || !std::filesystem::exists(path)) {
        return false;
    }

    std::wstring commandLine;
    if (std::filesystem::is_directory(path)) {
        commandLine = L"/e,\"" + path.wstring() + L"\"";
    } else {
        commandLine = L"/select,\"" + path.wstring() + L"\"";
    }

    SHELLEXECUTEINFOW executeInfo {};
    executeInfo.cbSize = sizeof(executeInfo);
    executeInfo.fMask = SEE_MASK_FLAG_NO_UI;
    executeInfo.lpVerb = L"open";
    executeInfo.lpFile = L"explorer.exe";
    executeInfo.lpParameters = commandLine.c_str();
    executeInfo.nShow = SW_SHOWNORMAL;
    return ShellExecuteExW(&executeInfo) == TRUE;
}

void EditorWorkspaceController::NotifyProjectStateChanged() const
{
    if (m_OnProjectStateChanged) {
        m_OnProjectStateChanged();
    }
}

int EditorWorkspaceController::FindDocumentIndexByPath(const std::filesystem::path& filePath) const
{
    if (filePath.empty()) {
        return -1;
    }

    std::error_code error;
    const std::filesystem::path normalizedTarget = std::filesystem::weakly_canonical(filePath, error);
    const std::filesystem::path targetPath = error ? filePath.lexically_normal() : normalizedTarget;

    for (int index = 0; index < static_cast<int>(m_Documents.size()); ++index) {
        const auto& document = m_Documents[static_cast<std::size_t>(index)].Session->GetDocument();
        if (!document || !document->HasFilePath()) {
            continue;
        }

        error.clear();
        const std::filesystem::path normalizedDocument =
            std::filesystem::weakly_canonical(document->GetFilePath(), error);
        const std::filesystem::path documentPath =
            error ? document->GetFilePath().lexically_normal() : normalizedDocument;
        if (documentPath == targetPath) {
            return index;
        }
    }

    return -1;
}

void EditorWorkspaceController::RememberRecentFile(const std::filesystem::path& filePath)
{
    if (filePath.empty()) {
        return;
    }

    std::error_code error;
    const std::filesystem::path normalizedTarget = std::filesystem::weakly_canonical(filePath, error);
    const std::filesystem::path targetPath = error ? filePath.lexically_normal() : normalizedTarget;

    m_RecentFiles.erase(
        std::remove_if(
            m_RecentFiles.begin(),
            m_RecentFiles.end(),
            [&targetPath](const std::filesystem::path& existing) {
                std::error_code compareError;
                const std::filesystem::path normalizedExisting =
                    std::filesystem::weakly_canonical(existing, compareError);
                const std::filesystem::path existingPath =
                    compareError ? existing.lexically_normal() : normalizedExisting;
                return existingPath == targetPath;
            }),
        m_RecentFiles.end());

    m_RecentFiles.insert(m_RecentFiles.begin(), targetPath);
    constexpr std::size_t kMaxRecentFiles = 10;
    if (m_RecentFiles.size() > kMaxRecentFiles) {
        m_RecentFiles.resize(kMaxRecentFiles);
    }
}

void EditorWorkspaceController::RemoveRecentFilesUnderPath(const std::filesystem::path& path)
{
    if (path.empty()) {
        return;
    }

    std::error_code error;
    const std::filesystem::path normalizedTarget = std::filesystem::weakly_canonical(path, error);
    const std::filesystem::path targetPath = error ? path.lexically_normal() : normalizedTarget;

    m_RecentFiles.erase(
        std::remove_if(
            m_RecentFiles.begin(),
            m_RecentFiles.end(),
            [&targetPath](const std::filesystem::path& recentPath) {
                std::error_code compareError;
                const std::filesystem::path normalizedRecent =
                    std::filesystem::weakly_canonical(recentPath, compareError);
                const std::filesystem::path recentResolved =
                    compareError ? recentPath.lexically_normal() : normalizedRecent;
                return recentResolved == targetPath || IsPathWithinRoot(recentResolved, targetPath);
            }),
        m_RecentFiles.end());
}

void EditorWorkspaceController::ReplaceRecentFilePath(const std::filesystem::path& oldPath, const std::filesystem::path& newPath)
{
    if (oldPath.empty() || newPath.empty()) {
        return;
    }

    for (std::filesystem::path& recentPath : m_RecentFiles) {
        if (AreEquivalentPaths(recentPath, oldPath)) {
            recentPath = newPath;
            continue;
        }

        if (IsPathWithinRoot(recentPath, oldPath)) {
            recentPath = newPath / std::filesystem::relative(recentPath, oldPath);
        }
    }
}

bool EditorWorkspaceController::CloseOpenDocumentsUnderPath(
    const std::filesystem::path& path,
    bool* outBlockedByDirtyDocument)
{
    if (outBlockedByDirtyDocument) {
        *outBlockedByDirtyDocument = false;
    }

    if (path.empty()) {
        return true;
    }

    std::error_code error;
    const std::filesystem::path normalizedTarget = std::filesystem::weakly_canonical(path, error);
    const std::filesystem::path targetPath = error ? path.lexically_normal() : normalizedTarget;

    for (const FDocumentEntry& entry : m_Documents) {
        if (!entry.Session || !entry.Session->GetDocument() || !entry.Session->GetDocument()->HasFilePath()) {
            continue;
        }

        const std::filesystem::path documentPath = entry.Session->GetDocument()->GetFilePath();
        error.clear();
        const std::filesystem::path normalizedDocument =
            std::filesystem::weakly_canonical(documentPath, error);
        const std::filesystem::path resolvedDocument =
            error ? documentPath.lexically_normal() : normalizedDocument;
        if (resolvedDocument != targetPath && !IsPathWithinRoot(resolvedDocument, targetPath)) {
            continue;
        }

        if (entry.Session->GetDocument()->IsDirty()) {
            if (outBlockedByDirtyDocument) {
                *outBlockedByDirtyDocument = true;
            }
            return false;
        }
    }

    for (int index = static_cast<int>(m_Documents.size()) - 1; index >= 0; --index) {
        const auto& entry = m_Documents[static_cast<std::size_t>(index)];
        if (!entry.Session || !entry.Session->GetDocument() || !entry.Session->GetDocument()->HasFilePath()) {
            continue;
        }

        const std::filesystem::path documentPath = entry.Session->GetDocument()->GetFilePath();
        error.clear();
        const std::filesystem::path normalizedDocument =
            std::filesystem::weakly_canonical(documentPath, error);
        const std::filesystem::path resolvedDocument =
            error ? documentPath.lexically_normal() : normalizedDocument;
        if (resolvedDocument != targetPath && !IsPathWithinRoot(resolvedDocument, targetPath)) {
            continue;
        }

        if (!FinalizeDocumentClose(index)) {
            return false;
        }
    }

    return true;
}

void EditorWorkspaceController::UpdateOpenDocumentPathsForRename(
    const std::filesystem::path& oldPath,
    const std::filesystem::path& newPath)
{
    if (oldPath.empty() || newPath.empty()) {
        return;
    }

    for (const FDocumentEntry& entry : m_Documents) {
        if (!entry.Session || !entry.Session->GetDocument() || !entry.Session->GetDocument()->HasFilePath()) {
            continue;
        }

        const std::filesystem::path documentPath = entry.Session->GetDocument()->GetFilePath();
        if (AreEquivalentPaths(documentPath, oldPath)) {
            entry.Session->UpdateDocumentFilePath(newPath);
            continue;
        }

        if (IsPathWithinRoot(documentPath, oldPath)) {
            entry.Session->UpdateDocumentFilePath(newPath / std::filesystem::relative(documentPath, oldPath));
        }
    }
}

void EditorWorkspaceController::RebuildProjectView()
{
    if (!m_ProjectView) {
        return;
    }

    CloseProjectItemContextMenu();
    m_ProjectItemBindings.clear();
    m_ProjectView->ClearItems();

    ImTextOutlineItem* openRootItem = m_ProjectView->AddRootItem("Open Documents");
    if (!openRootItem) {
        return;
    }

    openRootItem->Expanded = true;
    for (int index = 0; index < static_cast<int>(m_Documents.size()); ++index) {
        const auto& document = m_Documents[static_cast<std::size_t>(index)].Session->GetDocument();
        std::string label = m_Documents[static_cast<std::size_t>(index)].Session->GetDocumentTabTitle();
        if (document && document->HasFilePath()) {
            label += "  [" + document->GetFilePath().string() + "]";
        } else {
            label += "  [Unsaved]";
        }

        ImTextOutlineItem* item = m_ProjectView->AddChildItem(openRootItem, label);
        if (!item) {
            continue;
        }

        m_ProjectItemBindings[item] = FProjectItemBinding {EProjectItemKind::OpenDocument, index, {}};
        if (index == m_ActiveDocumentIndex) {
            m_ProjectView->SetSelectedItem(item);
        }
    }

    if (m_Documents.empty()) {
        ImTextOutlineItem* emptyItem = m_ProjectView->AddChildItem(openRootItem, "No open documents");
        if (emptyItem) {
            m_ProjectView->SetSelectedItem(emptyItem);
        }
    }

    ImTextOutlineItem* recentRootItem = m_ProjectView->AddRootItem("Recent Files");
    if (!recentRootItem) {
        return;
    }

    recentRootItem->Expanded = true;
    if (m_RecentFiles.empty()) {
        m_ProjectView->AddChildItem(recentRootItem, "No recent files");
    } else {
        for (const std::filesystem::path& filePath : m_RecentFiles) {
            const std::string label = filePath.filename().string() + "  [" + filePath.string() + "]";
            ImTextOutlineItem* item = m_ProjectView->AddChildItem(recentRootItem, label);
            if (!item) {
                continue;
            }

            m_ProjectItemBindings[item] = FProjectItemBinding {EProjectItemKind::RecentFile, -1, filePath};
        }
    }

    ImTextOutlineItem* workspaceRootItem = m_ProjectView->AddRootItem("Workspace");
    if (!workspaceRootItem) {
        return;
    }

    workspaceRootItem->Expanded = true;
    if (m_ProjectRoot.empty() || !std::filesystem::exists(m_ProjectRoot)) {
        m_ProjectView->AddChildItem(workspaceRootItem, "Workspace root not configured");
        return;
    }

    bool bAddedWorkspaceChild = false;
    std::function<void(ImTextOutlineItem*, const std::filesystem::path&, int)> addWorkspaceEntries =
        [this, &addWorkspaceEntries, &bAddedWorkspaceChild](ImTextOutlineItem* parentItem, const std::filesystem::path& directoryPath, int depth) {
            if (parentItem == nullptr || depth > 2) {
                return;
            }

            std::error_code iterateError;
            std::vector<std::filesystem::directory_entry> entries;
            for (const auto& entry : std::filesystem::directory_iterator(directoryPath, iterateError)) {
                if (iterateError) {
                    break;
                }
                if (ShouldSkipWorkspaceEntry(entry)) {
                    continue;
                }
                entries.push_back(entry);
            }

            std::sort(
                entries.begin(),
                entries.end(),
                [](const std::filesystem::directory_entry& left, const std::filesystem::directory_entry& right) {
                    if (left.is_directory() != right.is_directory()) {
                        return left.is_directory() && !right.is_directory();
                    }
                    return left.path().filename().string() < right.path().filename().string();
                });

            for (const auto& entry : entries) {
                const std::filesystem::path path = entry.path();
                if (entry.is_directory()) {
                    ImTextOutlineItem* directoryItem =
                        m_ProjectView->AddChildItem(parentItem, path.filename().string() + "/");
                    if (!directoryItem) {
                        continue;
                    }

                    bAddedWorkspaceChild = true;
                    directoryItem->Expanded = depth == 0;
                    m_ProjectItemBindings[directoryItem] =
                        FProjectItemBinding {EProjectItemKind::WorkspaceDirectory, -1, path};
                    addWorkspaceEntries(directoryItem, path, depth + 1);
                } else if (IsSupportedWorkspaceDocument(path)) {
                    ImTextOutlineItem* fileItem =
                        m_ProjectView->AddChildItem(parentItem, path.filename().string());
                    if (!fileItem) {
                        continue;
                    }

                    bAddedWorkspaceChild = true;
                    m_ProjectItemBindings[fileItem] =
                        FProjectItemBinding {EProjectItemKind::WorkspaceFile, -1, path};
                }
            }
        };
    addWorkspaceEntries(workspaceRootItem, m_ProjectRoot, 0);

    if (!bAddedWorkspaceChild) {
        m_ProjectView->AddChildItem(workspaceRootItem, "No supported files");
    }
}

void EditorWorkspaceController::HandleProjectSelectionChanged(ImTextOutlineView&, ImTextOutlineItem* item)
{
    if (item == nullptr) {
        return;
    }

    const auto it = m_ProjectItemBindings.find(item);
    if (it == m_ProjectItemBindings.end()) {
        return;
    }

    const FProjectItemBinding& binding = it->second;
    if (binding.Kind == EProjectItemKind::OpenDocument) {
        if (binding.Index != m_ActiveDocumentIndex) {
            ActivateDocumentAt(binding.Index);
        }
        return;
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory ||
        binding.Path.empty() ||
        !m_ShellHost) {
        return;
    }

    ImApplication* application = m_ShellHost->GetApplication();
    if (application == nullptr) {
        return;
    }

    const int existingIndex = FindDocumentIndexByPath(binding.Path);
    if (existingIndex >= 0) {
        ActivateDocumentAt(existingIndex);
        return;
    }

    if (binding.Kind == EProjectItemKind::RecentFile ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        OpenDocumentFromPath(binding.Path);
    }
}

} // namespace ImWidgetV4Editor
