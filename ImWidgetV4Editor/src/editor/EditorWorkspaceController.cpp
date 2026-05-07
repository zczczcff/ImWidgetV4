#include "EditorWorkspaceController.h"

#include "EditorSession.h"
#include "EditorShellHost.h"
#include "../inspector/ReflectionDetailsView.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>
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

} // namespace

EditorWorkspaceController::EditorWorkspaceController(
    std::function<std::shared_ptr<ImWidget>()> createDefaultDocumentRoot)
    : m_CreateDefaultDocumentRoot(std::move(createDefaultDocumentRoot))
{
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
}

void EditorWorkspaceController::RefreshProjectTree()
{
    RebuildProjectView();
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
    widgets.DesignerSurface = std::make_shared<ImDesignerSurface>();
    widgets.DocumentHost->SetContent(widgets.DesignerSurface);

    widgets.WorkspaceTabs = std::make_shared<ImTabView>();
    widgets.WorkspaceTabs->SetSupportsKeyboardFocus(true);
    widgets.WorkspaceTabs->SetStyle(CreateWorkspaceTabStyle());
    widgets.WorkspaceTabs->AddTab("Designer", widgets.DocumentHost);
    widgets.WorkspaceTabs->AddTab("Preview", widgets.PreviewHost);
    widgets.WorkspaceTabs->AddTab("Schema", widgets.SchemaText);

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
        m_OutputText);

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

void EditorWorkspaceController::ClosePendingPrompt()
{
    if (m_CloseConfirmWindow && m_ShellHost && m_ShellHost->GetApplication()) {
        m_ShellHost->GetApplication()->GetWindowManager().CloseWindow(m_CloseConfirmWindow);
    }

    m_CloseConfirmMenu.reset();
    m_CloseConfirmWindow.reset();
    m_PendingCloseDocumentIndex = -1;
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

void EditorWorkspaceController::RebuildProjectView()
{
    if (!m_ProjectView) {
        return;
    }

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
