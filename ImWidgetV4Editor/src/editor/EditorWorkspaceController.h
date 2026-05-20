#pragma once

#include "../build/BuildController.h"
#include "../toolchains/PlatformConfiguration.h"
#include <imwidgetv4/core/Application.h>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
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

namespace ImWidgetV4 {
class FProcessCancelToken;
}

namespace ImWidgetV4Editor {

struct FCreateAppProjectOptions;

class EditorProject;
class EditorDesignerSurfaceHost;
class EditorSession;
class EditorShellHost;
class InputDialog;
class NewAppProjectDialog;
class ProjectSettingsDialog;
struct FProjectScaffoldRequest;
class ReflectionDetailsView;

class EditorWorkspaceController : public std::enable_shared_from_this<EditorWorkspaceController> {
public:
    using json = nlohmann::ordered_json;

    explicit EditorWorkspaceController(
        std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> createDefaultDocumentRoot);
    ~EditorWorkspaceController();

    void SetOnProjectStateChanged(std::function<void()> callback);
    void SetOnExitRequested(std::function<void()> callback);
    void SetProjectRoot(const std::filesystem::path& projectRoot);
    const std::filesystem::path& GetProjectRoot() const { return m_ProjectRoot; }
    void RefreshProjectTree();
    void EnsureAtLeastOneSession();

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
    bool NewAppProject(ImWidgetV4::ImApplication& app);
    bool OpenAppProject(ImWidgetV4::ImApplication& app);
    bool OpenProjectSettings(ImWidgetV4::ImApplication& app);
    bool OpenAppProjectAt(const std::filesystem::path& projectRoot);
    bool ConfigureProject();
    bool ConfigureProject(const std::string& profileName);
    bool BuildProject();
    bool BuildProject(const std::string& profileName);
    bool RunProject();
    bool RunProject(const std::string& profileName);
    bool StopRunningProject();
    bool CleanProject();
    bool CleanProject(const std::string& profileName);
    bool ClearBuildCache();
    bool ClearBuildCache(const std::string& profileName);
    bool RebuildProject();
    bool RebuildProject(const std::string& profileName);
    void TickBackgroundTasks();
    bool IsBuildTaskRunning() const;
    bool IsRunTaskRunning() const;
    std::string GetBuildTaskStatusText() const;
    std::string GetActiveBuildProfileName() const;
    FEnvironmentProbeReport GetActiveBuildProfileProbeReport() const;
    bool TryGetBuildProfileProbeReport(const std::string& profileName, FEnvironmentProbeReport& outReport) const;
    bool IsBuildProfileProbeRefreshing(const std::string& profileName) const;
    bool IsActiveBuildProfileProbeRefreshing() const;
    void RequestBuildProfileProbeRefresh();
    std::vector<std::string> GetBuildProfileNames() const;
    bool SetActiveBuildProfile(const std::string& profileName);
    bool UpdateBuildProfile(const FEditorBuildProfile& profile, bool bMakeActive);
    bool UpdateProjectSettings(
        const FEditorBuildProfile& profile,
        bool bMakeActive,
        const FEditorApplicationSettings& applicationSettings);
    bool RevealProjectBuildDirectory() const;
    bool RevealProjectBuildDirectory(const std::string& profileName) const;
    bool CanRevealExecutableDirectoryForConfiguration(const std::string& configuration) const;
    bool RevealExecutableDirectoryForConfiguration(const std::string& configuration) const;
    std::filesystem::path ResolveExecutableDirectoryForConfiguration(const std::string& configuration) const;
    bool SelectProjectRoot(ImWidgetV4::ImApplication& app);
    bool RequestProjectRootChange(ImWidgetV4::ImApplication& app, const std::filesystem::path& projectRoot);
    bool CreateAppProjectAt(const std::filesystem::path& parentDirectory, const std::string& projectName);
    bool CreateAppProjectAt(const std::filesystem::path& parentDirectory, const FCreateAppProjectOptions& options);
    bool CreateDocumentInDirectory(ImWidgetV4::ImApplication& app, const std::filesystem::path& directoryPath);
    bool CreateFolderInDirectory(ImWidgetV4::ImApplication& app, const std::filesystem::path& directoryPath);
    bool CreateAndOpenDocumentAtPath(const std::filesystem::path& filePath);
    bool CreateFolderAtPath(const std::filesystem::path& directoryPath);
    bool RenameProjectItem(const std::filesystem::path& path, const std::string& newName);
    bool DeleteProjectItem(const std::filesystem::path& path);
    bool RevealProjectItemInExplorer(const std::filesystem::path& path) const;
    bool SaveDocument(ImWidgetV4::ImApplication& app);
    bool SaveDocumentAs(ImWidgetV4::ImApplication& app);
    bool GenerateActiveDocumentCpp(ImWidgetV4::ImApplication& app);
    bool RegenerateProjectCode();
    bool ReinitializeMainCpp();
    void PromptReinitializeMainCpp(ImWidgetV4::ImApplication& app);
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
    void BeginBatchUiUpdate();
    void EndBatchUiUpdate(bool bForceRefresh = true);
    int GetDocumentCount() const { return static_cast<int>(m_Documents.size()); }
    int GetActiveDocumentIndex() const { return m_ActiveDocumentIndex; }
    std::vector<std::string> GetOutputLines() const;

    std::shared_ptr<EditorSession> GetActiveSession() const;
    std::shared_ptr<EditorProject> GetProject() const { return m_Project; }

private:
    enum class EBackgroundBuildTaskKind {
        Configure,
        Build,
        Run,
        Clean,
        ClearCache,
        Rebuild
    };

    enum class EProjectItemKind {
        OpenDocument,
        RecentFile,
        WorkspaceDirectory,
        WorkspaceFile,
        BuildProfile
    };

    struct FSessionWidgets {
        std::shared_ptr<ImWidgetV4::ImWidget> Root;
        std::shared_ptr<ImWidgetV4::ImTabView> WorkspaceTabs;
        std::shared_ptr<ImWidgetV4::ImScrollBox> DocumentHost;
        std::shared_ptr<ImWidgetV4::ImScrollBox> PreviewHost;
        std::shared_ptr<ImWidgetV4::ImTextList> SchemaText;
        std::shared_ptr<ImWidgetV4::ImTextList> HeaderPreviewText;
        std::shared_ptr<ImWidgetV4::ImTextList> SourcePreviewText;
        std::shared_ptr<EditorDesignerSurfaceHost> DesignerHost;
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
        std::string ProfileName;
    };

    struct FBackgroundBuildTaskState {
        EBackgroundBuildTaskKind Kind = EBackgroundBuildTaskKind::Configure;
        std::string ProfileName;
        std::thread Worker;
        mutable std::mutex Mutex;
        std::vector<std::string> PendingOutputLines;
        FBuildResult Result;
        std::shared_ptr<ImWidgetV4::FProcessCancelToken> ProcessCancelToken;
        std::string StatusText;
        bool bStatusDirty = false;
        bool bFinished = false;
        bool bRefreshProjectTreeOnCompletion = false;
        bool bSawFetchContentCacheRemovalFailure = false;
        int LastReportedPercent = -1;
    };

    struct FBackgroundProbeTaskState {
        std::thread Worker;
        mutable std::mutex Mutex;
        std::unordered_map<std::string, FEnvironmentProbeReport> Results;
        bool bFinished = false;
    };

    std::shared_ptr<EditorSession> CreateSession() const;
    FSessionWidgets CreateSessionWidgets();
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
    void OpenCreateAppProjectDialog(
        ImWidgetV4::ImApplication& app,
        const std::filesystem::path& parentDirectory);
    void OpenProjectSettingsDialog(ImWidgetV4::ImApplication& app);
    void OpenCreateFolderDialog(
        ImWidgetV4::ImApplication& app,
        const std::filesystem::path& directoryPath);
    void PromptDeleteProjectItem(ImWidgetV4::ImApplication& app, const std::filesystem::path& path);
    void ClosePendingPrompt();
    bool BuildProjectScaffoldRequestForCurrentProject(FProjectScaffoldRequest& outRequest);
    json BuildWorkspaceStateJson() const;
    bool ApplyWorkspaceStateJson(const json& workspaceState, std::string* outError = nullptr);
    void OpenDocumentTabContextMenu(ImWidgetV4::ImApplication& app, int index, ImWidgetV4::FVector2 position);
    void CloseDocumentTabContextMenu();
    void OpenProjectItemContextMenu(ImWidgetV4::ImApplication& app, ImWidgetV4::ImTextOutlineItem* item, ImWidgetV4::FVector2 position);
    void CloseProjectItemContextMenu();
    void NotifyProjectStateChanged() const;
    void SetOutputLine(const std::string& text) const;
    void SetLocalizedOutputLine(
        const std::string& key,
        const std::string& defaultText,
        const std::string& suffix = std::string()) const;
    void AppendOutputLine(const std::string& text) const;
    void ReportSdkCompatibilityStatus() const;
    bool StartBackgroundBuildTask(EBackgroundBuildTaskKind kind, const std::string& profileName);
    void TickBackgroundBuildTask();
    void ShutdownBackgroundBuildTask();
    void InvalidateBuildProfileProbeCache();
    void StartBackgroundProbeTask();
    void TickBackgroundProbeTask();
    void ShutdownBackgroundProbeTask();
    int FindDocumentIndexByPath(const std::filesystem::path& filePath) const;
    void RememberRecentFile(const std::filesystem::path& filePath);
    void RemoveRecentFilesUnderPath(const std::filesystem::path& path);
    void ReplaceRecentFilePath(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    bool CloseOpenDocumentsUnderPath(const std::filesystem::path& path, bool* outBlockedByDirtyDocument = nullptr);
    void UpdateOpenDocumentPathsForRename(const std::filesystem::path& oldPath, const std::filesystem::path& newPath);
    bool HasDirtyDocuments() const;
    bool LoadProjectManifestAtRoot(const std::filesystem::path& projectRoot, bool bLogErrors = true);
    void ClearOpenDocuments();
    void RebuildProjectView();
    void RequestProjectViewRefresh();
    void HandleProjectSelectionChanged(ImWidgetV4::ImTextOutlineView& view, ImWidgetV4::ImTextOutlineItem* item);

    std::function<std::shared_ptr<ImWidgetV4::ImWidget>()> m_CreateDefaultDocumentRoot;
    std::function<void()> m_OnProjectStateChanged;
    std::function<void()> m_OnExitRequested;
    std::filesystem::path m_ProjectRoot;
    std::shared_ptr<EditorProject> m_Project;
    std::shared_ptr<EditorShellHost> m_ShellHost;
    std::shared_ptr<ImWidgetV4::ImTabView> m_DocumentTabs;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_ProjectView;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
    std::shared_ptr<ReflectionDetailsView> m_DetailsView;
    std::shared_ptr<ImWidgetV4::ImTextList> m_OutputText;
    std::shared_ptr<FBackgroundBuildTaskState> m_BackgroundBuildTask;
    std::shared_ptr<FBackgroundProbeTaskState> m_BackgroundProbeTask;
    std::unordered_map<ImWidgetV4::ImTextOutlineItem*, FProjectItemBinding> m_ProjectItemBindings;
    std::unordered_map<std::string, FEnvironmentProbeReport> m_BuildProfileProbeReports;
    std::unordered_set<std::string> m_RefreshingBuildProfileNames;
    std::vector<std::filesystem::path> m_RecentFiles;
    std::vector<FDocumentEntry> m_Documents;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_CloseConfirmMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_CloseConfirmWindow;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_DocumentTabContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_DocumentTabContextMenuWindow;
    std::shared_ptr<ImWidgetV4::ImPopupMenu> m_ProjectItemContextMenu;
    std::shared_ptr<ImWidgetV4::ImWindow> m_ProjectItemContextMenuWindow;
    std::shared_ptr<InputDialog> m_PendingInputDialog;
    std::shared_ptr<NewAppProjectDialog> m_PendingNewAppProjectDialog;
    std::shared_ptr<ProjectSettingsDialog> m_PendingProjectSettingsDialog;
    std::shared_ptr<ImWidgetV4::ImApplication> m_ExitPromptAppLock;
    std::filesystem::path m_PendingProjectRootChange;
    std::filesystem::path m_PendingCreateProjectParentPath;
    std::filesystem::path m_PendingRenameProjectItemPath;
    std::filesystem::path m_PendingDeleteProjectItemPath;
    int m_ContextMenuDocumentIndex = -1;
    ImWidgetV4::ImTextOutlineItem* m_ContextMenuProjectItem = nullptr;
    int m_PendingCloseDocumentIndex = -1;
    int m_ActiveDocumentIndex = -1;
    bool m_bBuildProfileProbeRefreshPending = false;
    bool m_bRefreshProjectViewOnProbeCompletion = false;
    bool m_bExitRequested = false;
    bool m_bIgnoringTabActivation = false;
    bool m_bBatchUiUpdateActive = false;
    bool m_bProjectViewRefreshPending = false;
    bool m_bProjectStateNotificationPending = false;
};

} // namespace ImWidgetV4Editor
