#pragma once

#include <imwidgetv4/core/Application.h>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
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

class EditorSession;
class EditorShellHost;
class ReflectionDetailsView;

class EditorWorkspaceController : public std::enable_shared_from_this<EditorWorkspaceController> {
public:
    explicit EditorWorkspaceController(
        std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);

    void SetProjectRoot(const std::filesystem::path& projectRoot);
    const std::filesystem::path& GetProjectRoot() const { return m_ProjectRoot; }
    void RefreshProjectTree();

    void Bind(
        const std::shared_ptr<EditorShellHost>& shellHost,
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& projectView,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& widgetTreeView,
        const std::shared_ptr<ReflectionDetailsView>& detailsView,
        const std::shared_ptr<ImWidgetV4::ImTextBlock>& outputText);

    bool NewDocument();
    bool OpenDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocumentAs(ImWidgetV4::ImApplication& app);
    bool CloseActiveDocument(ImWidgetV4::ImApplication& app);
    bool ActivateDocumentAt(int index);
    bool ActivateAdjacentDocument(int direction);
    bool CutSelectedWidget();
    bool CopySelectedWidget();
    bool PasteCopiedWidget();
    bool DuplicateSelectedWidget();
    bool Undo();
    bool Redo();

    std::shared_ptr<EditorSession> GetActiveSession() const;

private:
    enum class EProjectItemKind {
        OpenDocument,
        RecentFile,
        WorkspaceDirectory,
        WorkspaceFile
    };

    struct FSessionWidgets {
        std::shared_ptr<ImWidgetV4::ImWidget> Root;
        std::shared_ptr<ImWidgetV4::ImTabView> WorkspaceTabs;
        std::shared_ptr<ImWidgetV4::ImScrollBox> DocumentHost;
        std::shared_ptr<ImWidgetV4::ImScrollBox> PreviewHost;
        std::shared_ptr<ImWidgetV4::ImScrollBox> SchemaHost;
        std::shared_ptr<ImWidgetV4::ImTextBlock> SchemaText;
        std::shared_ptr<ImWidgetV4::ImDesignerSurface> DesignerSurface;
    };

    struct FDocumentEntry {
        std::shared_ptr<EditorSession> Session;
        FSessionWidgets Widgets;
    };

    struct FProjectItemBinding {
        EProjectItemKind Kind = EProjectItemKind::OpenDocument;
        int Index = -1;
        std::filesystem::path Path;
    };

    std::shared_ptr<EditorSession> CreateSession() const;
    FSessionWidgets CreateSessionWidgets() const;
    bool AddSession(const std::shared_ptr<EditorSession>& session, bool bActivateNewTab);
    bool OpenDocumentFromPath(const std::filesystem::path& filePath);
    bool CloseDocumentAt(ImWidgetV4::ImApplication& app, int index);
    void ActivateDocumentTab(int index);
    void HandleDocumentTabCloseRequested(ImWidgetV4::ImTabView& tabView, int index, bool& bAllowClose);
    void HandleDocumentTabClosed(ImWidgetV4::ImTabView& tabView, int closedIndex);
    void HandleDocumentTabContextMenuRequested(ImWidgetV4::ImTabView& tabView, int index, ImWidgetV4::FVector2 position);
    bool FinalizeDocumentClose(int index);
    bool CloseOtherDocuments(ImWidgetV4::ImApplication& app, int keepIndex);
    bool CloseAllDocuments(ImWidgetV4::ImApplication& app);
    void PromptCloseDirtyDocument(ImWidgetV4::ImApplication& app, int index);
    void ClosePendingPrompt();
    void OpenDocumentTabContextMenu(ImWidgetV4::ImApplication& app, int index, ImWidgetV4::FVector2 position);
    void CloseDocumentTabContextMenu();
    int FindDocumentIndexByPath(const std::filesystem::path& filePath) const;
    void RememberRecentFile(const std::filesystem::path& filePath);
    void RebuildProjectView();
    void HandleProjectSelectionChanged(ImWidgetV4::ImTextOutlineView& view, ImWidgetV4::ImTextOutlineItem* item);

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::filesystem::path m_ProjectRoot;
    std::shared_ptr<EditorShellHost> m_ShellHost;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_ProjectView;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
    std::shared_ptr<ReflectionDetailsView> m_DetailsView;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_OutputText;
    std::unordered_map<ImWidgetV4::ImTextOutlineItem*, FProjectItemBinding> m_ProjectItemBindings;
    std::vector<std::filesystem::path> m_RecentFiles;
    std::vector<FDocumentEntry> m_Documents;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_CloseConfirmMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_CloseConfirmWindow;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_DocumentTabContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_DocumentTabContextMenuWindow;
    int m_ContextMenuDocumentIndex = -1;
    int m_PendingCloseDocumentIndex = -1;
    int m_ActiveDocumentIndex = -1;
    bool m_bIgnoringTabActivation = false;
};

} // namespace ImWidgetV4Editor
