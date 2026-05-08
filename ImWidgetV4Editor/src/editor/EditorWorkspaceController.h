#pragma once

#include <imwidgetv4/core/Application.h>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
class ImDesignerSurface;
class ImButton;
class ImEditableText;
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

class EditorSession;
class EditorShellHost;
class InputDialog;
class ReflectionDetailsView;

class EditorWorkspaceController : public std::enable_shared_from_this<EditorWorkspaceController> {
public:
    using json = nlohmann::ordered_json;

    explicit EditorWorkspaceController(
        std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);

    void SetOnProjectStateChanged(std::function<void()> callback);
    void SetOnExitRequested(std::function<void()> callback);
    void SetProjectRoot(const std::filesystem::path& projectRoot);
    const std::filesystem::path& GetProjectRoot() const { return m_ProjectRoot; }
    void RefreshProjectTree();

    void Bind(
        const std::shared_ptr<EditorShellHost>& shellHost,
        const std::shared_ptr<ImWidgetV4::ImTabView>& documentTabs,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& projectView,
        const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& widgetTreeView,
        const std::shared_ptr<ReflectionDetailsView>& detailsView,
        const std::shared_ptr<ImWidgetV4::ImTextList>& outputText);

    bool NewDocument();
    bool OpenDocument(ImWidgetV4::ImApplication& app);
    bool OpenDocumentFromPath(const std::filesystem::path& filePath);
    bool SelectProjectRoot(ImWidgetV4::ImApplication& app);
    bool RequestProjectRootChange(ImWidgetV4::ImApplication& app, const std::filesystem::path& projectRoot);
    bool CreateDocumentInDirectory(ImWidgetV4::ImApplication& app, const std::filesystem::path& directoryPath);
    bool CreateFolderInDirectory(ImWidgetV4::ImApplication& app, const std::filesystem::path& directoryPath);
    bool CreateAndOpenDocumentAtPath(const std::filesystem::path& filePath);
    bool CreateFolderAtPath(const std::filesystem::path& directoryPath);
    bool RenameProjectItem(const std::filesystem::path& path, const std::string& newName);
    bool DeleteProjectItem(const std::filesystem::path& path);
    bool RevealProjectItemInExplorer(const std::filesystem::path& path) const;
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
    bool RequestApplicationClose(ImWidgetV4::ImApplication& app);
    void ConfirmApplicationClose();
    bool SaveWorkspaceState(const std::filesystem::path& filePath) const;
    bool LoadWorkspaceState(const std::filesystem::path& filePath);
    int GetDocumentCount() const { return static_cast<int>(m_Documents.size()); }
    int GetActiveDocumentIndex() const { return m_ActiveDocumentIndex; }

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
        std::shared_ptr<ImWidgetV4::ImTextList> SchemaText;
        std::shared_ptr<ImWidgetV4::ImTextList> HeaderPreviewText;
        std::shared_ptr<ImWidgetV4::ImTextList> SourcePreviewText;
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
    bool CloseDocumentAt(ImWidgetV4::ImApplication& app, int index);
    void ActivateDocumentTab(int index);
    void HandleDocumentTabCloseRequested(ImWidgetV4::ImTabView& tabView, int index, bool& bAllowClose);
    void HandleDocumentTabClosed(ImWidgetV4::ImTabView& tabView, int closedIndex);
    void HandleDocumentTabContextMenuRequested(ImWidgetV4::ImTabView& tabView, int index, ImWidgetV4::FVector2 position);
    void HandleProjectItemContextMenuRequested(
        ImWidgetV4::ImTextOutlineView& view,
        ImWidgetV4::ImTextOutlineItem& item,
        ImWidgetV4::FVector2 position);
    bool FinalizeDocumentClose(int index);
    bool CloseOtherDocuments(ImWidgetV4::ImApplication& app, int keepIndex);
    bool CloseAllDocuments(ImWidgetV4::ImApplication& app);
    void PromptCloseDirtyDocument(ImWidgetV4::ImApplication& app, int index);
    void PromptExitWithDirtyDocuments(ImWidgetV4::ImApplication& app);
    void PromptProjectRootChangeWithDirtyDocuments(
        ImWidgetV4::ImApplication& app,
        const std::filesystem::path& projectRoot);
    void OpenRenameProjectItemDialog(
        ImWidgetV4::ImApplication& app,
        const std::filesystem::path& path);
    void OpenCreateDocumentDialog(
        ImWidgetV4::ImApplication& app,
        const std::filesystem::path& directoryPath);
    void OpenCreateFolderDialog(
        ImWidgetV4::ImApplication& app,
        const std::filesystem::path& directoryPath);
    void PromptDeleteProjectItem(ImWidgetV4::ImApplication& app, const std::filesystem::path& path);
    void ClosePendingPrompt();
    json BuildWorkspaceStateJson() const;
    bool ApplyWorkspaceStateJson(const json& workspaceState, std::string* outError = nullptr);
    void OpenDocumentTabContextMenu(ImWidgetV4::ImApplication& app, int index, ImWidgetV4::FVector2 position);
    void CloseDocumentTabContextMenu();
    void OpenProjectItemContextMenu(ImWidgetV4::ImApplication& app, ImWidgetV4::ImTextOutlineItem* item, ImWidgetV4::FVector2 position);
    void CloseProjectItemContextMenu();
    void NotifyProjectStateChanged() const;
    int FindDocumentIndexByPath(const std::filesystem::path& filePath) const;
    void RememberRecentFile(const std::filesystem::path& filePath);
    void RemoveRecentFilesUnderPath(const std::filesystem::path& path);
    void ReplaceRecentFilePath(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    bool CloseOpenDocumentsUnderPath(const std::filesystem::path& path, bool* outBlockedByDirtyDocument = nullptr);
    void UpdateOpenDocumentPathsForRename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    void RebuildProjectView();
    void HandleProjectSelectionChanged(ImWidgetV4::ImTextOutlineView& view, ImWidgetV4::ImTextOutlineItem* item);

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::function<void()> m_OnProjectStateChanged;
    std::function<void()> m_OnExitRequested;
    std::filesystem::path m_ProjectRoot;
    std::shared_ptr<EditorShellHost> m_ShellHost;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_ProjectView;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
    std::shared_ptr<ReflectionDetailsView> m_DetailsView;
    std::shared_ptr<ImWidgetV4::ImTextList> m_OutputText;
    std::unordered_map<ImWidgetV4::ImTextOutlineItem*, FProjectItemBinding> m_ProjectItemBindings;
    std::vector<std::filesystem::path> m_RecentFiles;
    std::vector<FDocumentEntry> m_Documents;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_CloseConfirmMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_CloseConfirmWindow;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_DocumentTabContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_DocumentTabContextMenuWindow;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_ProjectItemContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_ProjectItemContextMenuWindow;
    std::shared_ptr<InputDialog> m_PendingInputDialog;
    std::shared_ptr<ImWidgetV4::ImApplication> m_ExitPromptAppLock;
    std::filesystem::path m_PendingProjectRootChange;
    std::filesystem::path m_PendingRenameProjectItemPath;
    std::filesystem::path m_PendingDeleteProjectItemPath;
    int m_ContextMenuDocumentIndex = -1;
    ImWidgetV4::ImTextOutlineItem* m_ContextMenuProjectItem = nullptr;
    int m_PendingCloseDocumentIndex = -1;
    int m_ActiveDocumentIndex = -1;
    bool m_bExitRequested = false;
    bool m_bIgnoringTabActivation = false;
};

} // namespace ImWidgetV4Editor
