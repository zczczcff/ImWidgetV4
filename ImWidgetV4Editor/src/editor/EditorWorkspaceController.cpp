#include "EditorWorkspaceController.h"

#include "EditorLocalization.h"
#include "EditorSession.h"
#include "EditorPaths.h"
#include "EditorDesignerSurfaceHost.h"
#include "EditorShellHost.h"
#include "EditorTheme.h"
#include "InputDialog.h"
#include "NewAppProjectDialog.h"
#include "ProjectSettingsDialog.h"
#include "../build/BuildController.h"
#include "../project/EditorProject.h"
#include "../templates/ProjectScaffolder.h"
#include "../inspector/ReflectionDetailsView.h"
#include "../inspector/PropertyEditorWidgets.h"
#include "../toolchains/EnvironmentProbe.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/platform/PlatformProcess.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>
#include <cctype>
#include <cmath>
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
    title->SetTextColor(GetEditorPanelTitleColor());
    return title;
}

std::shared_ptr<ImTextBlock> MakePanelBody(const std::string& text, float fontSize = 14.0f)
{
    auto body = std::make_shared<ImTextBlock>();
    body->SetText(text);
    body->SetWrapText(false);
    body->SetFontSize(fontSize);
    body->SetTextColor(GetEditorPanelBodyColor());
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

std::string LocalizedEditorString(
    const std::string& key,
    const std::string& defaultText,
    const std::string& suffix = std::string())
{
    return EditorText(key, defaultText).Resolve() + suffix;
}

std::shared_ptr<ImScrollBox> CreateDocumentHost()
{
    auto documentHost = std::make_shared<ImScrollBox>();
    documentHost->SetStyle(MakeEditorHostScrollStyle(documentHost->GetStyle(), FMargin(0.0f)));
    return documentHost;
}

std::shared_ptr<ImScrollBox> CreatePreviewHost()
{
    auto previewHost = std::make_shared<ImScrollBox>();
    previewHost->SetStyle(MakeEditorHostScrollStyle(previewHost->GetStyle(), FMargin(0.0f)));
    return previewHost;
}

std::shared_ptr<ImTextList> CreateSchemaText()
{
    auto schemaText = std::make_shared<ImTextList>();
    schemaText->SetStyle(MakeEditorCodeTextListStyle(schemaText->GetStyle(), FMargin(12.0f), FVector2(0.0f, 180.0f)));
    schemaText->SetItems({"{}"});
    return schemaText;
}

std::shared_ptr<ImTextList> CreateCodePreviewText()
{
    auto previewText = std::make_shared<ImTextList>();
    previewText->SetStyle(MakeEditorCodeTextListStyle(previewText->GetStyle(), FMargin(12.0f), FVector2(0.0f, 180.0f)));
    previewText->SetItems({EditorText("Session.NoDocumentLoadedComment", "// No document loaded.")});
    return previewText;
}

FTabViewStyle CreateWorkspaceTabStyle()
{
    return MakeEditorWorkspaceTabStyle();
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
    const std::string fileName = filePath.filename().string();
    if (fileName == "CMakeLists.txt") {
        return true;
    }

    const std::string extension = filePath.extension().string();
    return extension == ".json" ||
           extension == ".ui" ||
           extension == ".txt" ||
           extension == ".h" ||
           extension == ".hpp" ||
           extension == ".cpp" ||
           extension == ".cxx" ||
           extension == ".cmake";
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

bool IsIdentifierStartChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalpha(value) != 0 || c == '_';
}

bool IsIdentifierContinueChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalnum(value) != 0 || c == '_';
}

std::string TrimWhitespaceCopy(const std::string& text)
{
    const std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }

    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string NormalizeProjectIdentifier(const std::string& rawText, const std::string& fallback)
{
    const std::string source = TrimWhitespaceCopy(rawText);
    const std::string fallbackSource = TrimWhitespaceCopy(fallback).empty()
        ? std::string("AppProject")
        : TrimWhitespaceCopy(fallback);

    auto sanitize = [](const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (char c : text) {
            if (IsIdentifierContinueChar(c)) {
                result.push_back(c);
            } else if (result.empty() || result.back() != '_') {
                result.push_back('_');
            }
        }

        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }

        return result;
    };

    std::string normalized = sanitize(source);
    if (normalized.empty()) {
        normalized = sanitize(fallbackSource);
    }
    if (normalized.empty()) {
        normalized = "AppProject";
    }
    if (!IsIdentifierStartChar(normalized.front())) {
        normalized.insert(normalized.begin(), '_');
    }
    return normalized;
}

std::vector<std::filesystem::path> BuildExecutableCandidatePaths(
    const std::filesystem::path& buildDirectory,
    const std::string& configuration,
    const std::string& executableName)
{
    return {
        buildDirectory / configuration / (executableName + ".exe"),
        buildDirectory / (executableName + ".exe"),
        buildDirectory / "bin" / configuration / (executableName + ".exe"),
        buildDirectory / "bin" / (executableName + ".exe")
    };
}

std::filesystem::path FindBuiltExecutablePath(
    const EditorProject& project,
    const FEditorBuildProfile& profile)
{
    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), profile);
    const std::string executableName = NormalizeProjectIdentifier(project.GetProjectName(), "ImWidgetApp");
    for (const std::filesystem::path& candidate : BuildExecutableCandidatePaths(
             buildDirectory,
             profile.Configuration,
             executableName)) {
        std::error_code errorCode;
        if (std::filesystem::is_regular_file(candidate, errorCode)) {
            return candidate.lexically_normal();
        }
    }

    return {};
}

std::filesystem::path ResolveExecutableDirectoryPath(
    const EditorProject& project,
    const FEditorBuildProfile& profile)
{
    if (profile.TargetPlatform != EEditorTargetPlatform::WindowsDesktop) {
        return {};
    }

    if (const std::filesystem::path executablePath = FindBuiltExecutablePath(project, profile);
        !executablePath.empty()) {
        return executablePath.parent_path().lexically_normal();
    }

    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), profile);
    const std::string executableName = NormalizeProjectIdentifier(project.GetProjectName(), "ImWidgetApp");
    for (const std::filesystem::path& candidate : BuildExecutableCandidatePaths(
             buildDirectory,
             profile.Configuration,
             executableName)) {
        const std::filesystem::path parentPath = candidate.parent_path();
        std::error_code errorCode;
        if (!parentPath.empty() && std::filesystem::is_directory(parentPath, errorCode)) {
            return parentPath.lexically_normal();
        }
    }

    return {};
}

const FEditorBuildProfile* FindWindowsBuildProfileForConfiguration(
    const EditorProject& project,
    const std::string& configuration)
{
    for (const FEditorBuildProfile& profile : project.GetBuildProfiles()) {
        if (profile.TargetPlatform == EEditorTargetPlatform::WindowsDesktop &&
            profile.Configuration == configuration) {
            return &profile;
        }
    }

    return nullptr;
}

bool ContainsPathSeparators(const std::string& text)
{
    return text.find('/') != std::string::npos || text.find('\\') != std::string::npos;
}

std::string NormalizeStartupDocumentFileName(const std::string& rawText)
{
    std::string trimmedName = TrimWhitespaceCopy(rawText);
    if (trimmedName.empty()) {
        trimmedName = "Main";
    }

    if (EndsWithCaseInsensitive(trimmedName, ".ui.json")) {
        return trimmedName;
    }

    std::filesystem::path path(trimmedName);
    const std::string stem = path.stem().string();
    const std::string baseName = stem.empty() ? trimmedName : stem;
    return baseName + ".ui.json";
}

std::string GetDocumentDisplayTitleFromFileName(const std::string& fileName)
{
    std::string title = fileName;
    if (EndsWithCaseInsensitive(title, ".ui.json")) {
        title.resize(title.size() - std::string(".ui.json").size());
    } else {
        title = std::filesystem::path(title).stem().string();
    }

    return title.empty() ? std::string("Main") : title;
}

std::string BuildStartupWidgetClassName(const std::string& startupDocumentFileName)
{
    std::string baseName = startupDocumentFileName;
    if (EndsWithCaseInsensitive(baseName, ".ui.json")) {
        baseName.resize(baseName.size() - std::string(".ui.json").size());
    } else {
        baseName = std::filesystem::path(baseName).stem().string();
    }

    std::string className = NormalizeProjectIdentifier(baseName, "MainView");
    if (!className.empty()) {
        className.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(className.front())));
    }
    if (className.size() < 4 || className.substr(className.size() - 4) != "View") {
        className += "View";
    }
    return className;
}

std::shared_ptr<ImTitleBar> BuildDefaultTitleBarRoot(const std::string& projectName)
{
    auto titleBar = std::make_shared<ImTitleBar>();
    titleBar->SetName("RootTitleBar");
    titleBar->SetShowSystemButtons(true);

    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetName("ProjectTitleText");
    titleText->SetText(projectName.empty() ? std::string("Application") : projectName);
    titleText->SetWrapText(false);
    titleBar->AddLeadingItem(titleText);

    return titleBar;
}

std::vector<std::string> GetAvailableProjectTemplateNames()
{
    return {"Blank App"};
}

FPopupMenuItem MakeEditorMenuItem(
    const std::string& key,
    const std::string& defaultText,
    bool bEnabled,
    std::function<void()> onInvoked)
{
    FPopupMenuItem item;
    item.Text = defaultText;
    item.TextValue = EditorText(key, defaultText);
    item.bEnabled = bEnabled;
    item.OnInvoked = std::move(onInvoked);
    return item;
}

FPopupMenuItem MakeEditorMenuItem(
    const FText& text,
    bool bEnabled,
    std::function<void()> onInvoked)
{
    FPopupMenuItem item;
    item.Text = text.GetInvariantText();
    item.TextValue = text;
    item.bEnabled = bEnabled;
    item.OnInvoked = std::move(onInvoked);
    return item;
}

std::string BuildBackgroundTaskDisplayName(int kind)
{
    switch (kind) {
    case 0:
        return EditorText("Build.Configure", "Configure").Resolve();
    case 1:
        return EditorText("Build.Build", "Build").Resolve();
    case 2:
        return EditorText("Build.Run", "Run").Resolve();
    case 3:
        return EditorText("Build.Clean", "Clean").Resolve();
    case 4:
        return EditorText("Build.ClearCache", "Clear Cache").Resolve();
    case 5:
        return EditorText("Build.Rebuild", "Rebuild").Resolve();
    default:
        return EditorText("Build.Build", "Build").Resolve();
    }
}

bool StartsWith(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size() &&
           std::equal(prefix.begin(), prefix.end(), text.begin());
}

bool ContainsIgnoreCase(const std::string& text, const std::string& pattern)
{
    if (pattern.empty() || text.size() < pattern.size()) {
        return false;
    }

    auto toLower = [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    };

    for (std::size_t offset = 0; offset + pattern.size() <= text.size(); ++offset) {
        bool bMatch = true;
        for (std::size_t index = 0; index < pattern.size(); ++index) {
            if (toLower(static_cast<unsigned char>(text[offset + index])) !=
                toLower(static_cast<unsigned char>(pattern[index]))) {
                bMatch = false;
                break;
            }
        }
        if (bMatch) {
            return true;
        }
    }

    return false;
}

std::string ToLowerAscii(std::string text)
{
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return text;
}

bool IsFetchContentCacheRemovalFailureLine(const std::string& line)
{
    const std::string lowerLine = ToLowerAscii(line);
    return lowerLine.find("error removing directory") != std::string::npos &&
           lowerLine.find("_deps") != std::string::npos &&
           lowerLine.find("-src") != std::string::npos;
}

bool IsBuildDirectoryInsideProjectRoot(
    const std::filesystem::path& projectRoot,
    const std::filesystem::path& buildDirectory)
{
    if (projectRoot.empty() || buildDirectory.empty()) {
        return false;
    }

    const std::filesystem::path normalizedProjectRoot = projectRoot.lexically_normal();
    const std::filesystem::path normalizedBuildDirectory = buildDirectory.lexically_normal();
    if (normalizedProjectRoot == normalizedBuildDirectory) {
        return false;
    }

    auto rootIterator = normalizedProjectRoot.begin();
    auto buildIterator = normalizedBuildDirectory.begin();
    for (; rootIterator != normalizedProjectRoot.end(); ++rootIterator, ++buildIterator) {
        if (buildIterator == normalizedBuildDirectory.end() || *rootIterator != *buildIterator) {
            return false;
        }
    }

    return true;
}

FBuildResult ClearBuildDirectoryCache(
    const EditorProject& project,
    const FEditorBuildProfile& profile,
    const BuildController::FOutputCallback& outputCallback)
{
    FBuildResult result;
    result.BuildDirectory = ResolveBuildDirectoryPath(project.GetProjectRoot(), profile);

    if (!IsBuildDirectoryInsideProjectRoot(project.GetProjectRoot(), result.BuildDirectory)) {
        result.ErrorMessage = EditorText("Build.ClearCacheRefusedOutsideProject", "Clear cache refused: build directory is outside the project root.").Resolve();
        return result;
    }

    if (outputCallback) {
        outputCallback("[clear-cache:" + profile.Name + "] " + result.BuildDirectory.string());
    }

    std::error_code errorCode;
    if (!std::filesystem::exists(result.BuildDirectory, errorCode)) {
        result.bSuccess = true;
        result.ExitCode = 0;
        if (outputCallback) {
            outputCallback(EditorText("Build.ClearCacheDirectoryAlreadyAbsent", "Build cache directory does not exist.").Resolve());
        }
        return result;
    }

    std::filesystem::remove_all(result.BuildDirectory, errorCode);
    if (errorCode) {
        result.ErrorMessage =
            EditorText("Build.ClearCacheFailed", "Clear cache failed").Resolve() + ": " + errorCode.message();
        return result;
    }

    result.bSuccess = true;
    result.ExitCode = 0;
    if (outputCallback) {
        outputCallback(EditorText("Build.ClearCacheComplete", "Build cache directory removed.").Resolve());
    }
    return result;
}

bool TryParseBracketProgress(const std::string& line, int& outCurrent, int& outTotal)
{
    outCurrent = 0;
    outTotal = 0;

    for (std::size_t index = 0; index < line.size(); ++index) {
        if (line[index] != '[') {
            continue;
        }

        std::size_t currentBegin = index + 1;
        std::size_t currentEnd = currentBegin;
        while (currentEnd < line.size() && std::isdigit(static_cast<unsigned char>(line[currentEnd])) != 0) {
            ++currentEnd;
        }
        if (currentEnd == currentBegin || currentEnd >= line.size() || line[currentEnd] != '/') {
            continue;
        }

        std::size_t totalBegin = currentEnd + 1;
        std::size_t totalEnd = totalBegin;
        while (totalEnd < line.size() && std::isdigit(static_cast<unsigned char>(line[totalEnd])) != 0) {
            ++totalEnd;
        }
        if (totalEnd == totalBegin || totalEnd >= line.size() || line[totalEnd] != ']') {
            continue;
        }

        try {
            outCurrent = std::stoi(line.substr(currentBegin, currentEnd - currentBegin));
            outTotal = std::stoi(line.substr(totalBegin, totalEnd - totalBegin));
        } catch (...) {
            return false;
        }

        return outCurrent >= 0 && outTotal > 0 && outCurrent <= outTotal;
    }

    return false;
}

std::string BuildProgressStatusText(int current, int total)
{
    const int percent = total > 0
        ? static_cast<int>(std::lround((static_cast<double>(current) * 100.0) / static_cast<double>(total)))
        : 0;
    std::ostringstream stream;
    stream << "Building... " << percent << "% (" << current << "/" << total << ")";
    return stream.str();
}

template<typename TTaskState>
void UpdateBackgroundBuildTaskStatus(
    const std::shared_ptr<TTaskState>& task,
    const std::string& statusText)
{
    if (!task || statusText.empty()) {
        return;
    }

    if (task->StatusText == statusText) {
        return;
    }

    task->StatusText = statusText;
    task->bStatusDirty = true;
}

template<typename TTaskState>
void HandleBackgroundBuildOutputLine(
    const std::shared_ptr<TTaskState>& task,
    const std::string& line)
{
    if (!task || line.empty()) {
        return;
    }

    if (ContainsIgnoreCase(line, "Performing download step")) {
        UpdateBackgroundBuildTaskStatus(task, "Downloading third-party dependencies...");
        return;
    }

    if (ContainsIgnoreCase(line, "Performing update step")) {
        UpdateBackgroundBuildTaskStatus(task, "Updating third-party dependencies...");
        return;
    }

    if (ContainsIgnoreCase(line, "Performing patch step")) {
        UpdateBackgroundBuildTaskStatus(task, "Patching third-party dependencies...");
        return;
    }

    if (ContainsIgnoreCase(line, "Performing configure step")) {
        UpdateBackgroundBuildTaskStatus(task, "Configuring dependency project...");
        return;
    }

    if (ContainsIgnoreCase(line, "Configuring done")) {
        UpdateBackgroundBuildTaskStatus(task, "Generating project files...");
        return;
    }

    if (ContainsIgnoreCase(line, "Generating done")) {
        UpdateBackgroundBuildTaskStatus(task, "Generating project files...");
        return;
    }

    if (ContainsIgnoreCase(line, "Build files have been written to")) {
        UpdateBackgroundBuildTaskStatus(task, "Generating project files...");
        return;
    }

    if (StartsWith(line, "[configure")) {
        UpdateBackgroundBuildTaskStatus(task, "Configuring project...");
        return;
    }

    if (StartsWith(line, "[build")) {
        UpdateBackgroundBuildTaskStatus(task, "Starting build...");
        return;
    }

    if (StartsWith(line, "[run")) {
        UpdateBackgroundBuildTaskStatus(task, "Running application...");
        return;
    }

    if (StartsWith(line, "[clean")) {
        UpdateBackgroundBuildTaskStatus(task, "Cleaning project...");
        return;
    }

    if (StartsWith(line, "[clear-cache")) {
        UpdateBackgroundBuildTaskStatus(task, "Clearing build cache...");
        return;
    }

    if (IsFetchContentCacheRemovalFailureLine(line)) {
        task->bSawFetchContentCacheRemovalFailure = true;
    }

    int current = 0;
    int total = 0;
    if (TryParseBracketProgress(line, current, total)) {
        const int percent = static_cast<int>(std::lround((static_cast<double>(current) * 100.0) / static_cast<double>(total)));
        if (percent != task->LastReportedPercent) {
            task->LastReportedPercent = percent;
            UpdateBackgroundBuildTaskStatus(task, BuildProgressStatusText(current, total));
        }
    }
}

} // namespace

EditorWorkspaceController::EditorWorkspaceController(
    std::function<std::shared_ptr<ImWidget>()> createDefaultDocumentRoot)
    : m_CreateDefaultDocumentRoot(std::move(createDefaultDocumentRoot))
{
}

EditorWorkspaceController::~EditorWorkspaceController()
{
    ShutdownBackgroundBuildTask();
    ShutdownBackgroundProbeTask();
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
    InvalidateBuildProfileProbeCache();
    if (projectRoot.empty()) {
        m_ProjectRoot.clear();
        m_Project.reset();
    } else {
        std::error_code error;
        const std::filesystem::path canonicalPath =
            std::filesystem::weakly_canonical(projectRoot, error);
        m_ProjectRoot = error ? projectRoot.lexically_normal() : canonicalPath;
        LoadProjectManifestAtRoot(m_ProjectRoot, false);
    }

    RequestProjectViewRefresh();
    NotifyProjectStateChanged();
}

void EditorWorkspaceController::RefreshProjectTree()
{
    RequestProjectViewRefresh();
    NotifyProjectStateChanged();
}

void EditorWorkspaceController::EnsureAtLeastOneSession()
{
    if (!m_Documents.empty()) {
        return;
    }

    BeginBatchUiUpdate();
    AddSession(CreateSession(), true);
    EndBatchUiUpdate(true);
}

void EditorWorkspaceController::BeginBatchUiUpdate()
{
    m_bBatchUiUpdateActive = true;
}

void EditorWorkspaceController::EndBatchUiUpdate(bool bForceRefresh)
{
    m_bBatchUiUpdateActive = false;

    if (bForceRefresh || m_bProjectViewRefreshPending) {
        RebuildProjectView();
        m_bProjectViewRefreshPending = false;
    }

    if (bForceRefresh || m_bProjectStateNotificationPending) {
        m_bProjectStateNotificationPending = false;
        if (m_OnProjectStateChanged) {
            m_OnProjectStateChanged();
        }
    }
}

bool EditorWorkspaceController::SelectProjectRoot(ImApplication& app)
{
    FOpenFolderDialogOptions options;
    options.Title = EditorText("Workspace.SelectProjectRoot", "Select Project Root").Resolve();
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Unsupported && m_OutputText) {
            m_OutputText->SetItems({EditorText("Workspace.SelectProjectRootUnsupported", "Select project root is unsupported by the active platform backend.")});
        } else if (dialogResult.Code == EPathDialogResultCode::Error && m_OutputText) {
            m_OutputText->SetItems({FText::FromString(EditorText("Workspace.SelectProjectRootFailed", "Select project root failed").Resolve() + ": " + dialogResult.ErrorMessage)});
        }
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
        m_OutputText->SetItems({FText::FromString(EditorText("Workspace.ProjectRoot", "Project root").Resolve() + ": " + normalizedTarget.string())});
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

bool EditorWorkspaceController::CreateAppProjectAt(
    const std::filesystem::path& parentDirectory,
    const std::string& projectName)
{
    FCreateAppProjectOptions options;
    options.ProjectName = projectName;
    options.NamespaceName = NormalizeProjectIdentifier(projectName, "AppProject");
    options.StartupDocumentName = "Main";
    options.TemplateName = "Blank App";
    return CreateAppProjectAt(parentDirectory, options);
}

bool EditorWorkspaceController::CreateAppProjectAt(
    const std::filesystem::path& parentDirectory,
    const FCreateAppProjectOptions& options)
{
    const std::string trimmedProjectName = TrimWhitespaceCopy(options.ProjectName);
    const std::filesystem::path folderNamePath = std::filesystem::path(trimmedProjectName).filename();
    if (parentDirectory.empty() || trimmedProjectName.empty() || folderNamePath != trimmedProjectName) {
        SetLocalizedOutputLine("NewProject.ProjectNamePathSeparator", "Create project failed: project name must not contain path separators.");
        return false;
    }

    const std::string trimmedNamespaceName = TrimWhitespaceCopy(options.NamespaceName);
    if (trimmedNamespaceName.empty()) {
        SetLocalizedOutputLine("NewProject.NamespaceEmpty", "Create project failed: namespace must not be empty.");
        return false;
    }

    const std::string normalizedStartupDocumentFileName =
        NormalizeStartupDocumentFileName(options.StartupDocumentName);
    if (normalizedStartupDocumentFileName.empty() ||
        ContainsPathSeparators(normalizedStartupDocumentFileName) ||
        std::filesystem::path(normalizedStartupDocumentFileName).filename().string() != normalizedStartupDocumentFileName) {
        SetLocalizedOutputLine("NewProject.StartupUIPathSeparator", "Create project failed: startup UI name must not contain path separators.");
        return false;
    }

    const std::string trimmedTemplateName = TrimWhitespaceCopy(options.TemplateName).empty()
        ? std::string("Blank App")
        : TrimWhitespaceCopy(options.TemplateName);

    if (HasDirtyDocuments()) {
        SetLocalizedOutputLine("Project.CreateBlockedDirtyDocuments", "Create project blocked: save or discard dirty documents first.");
        return false;
    }

    try {
        const std::filesystem::path projectRoot = (parentDirectory / folderNamePath).lexically_normal();
        if (std::filesystem::exists(projectRoot)) {
            SetLocalizedOutputLine("NewProject.TargetFolderExists", "Create project failed: target folder already exists.");
            return false;
        }

        std::filesystem::create_directories(projectRoot / "src");
        std::filesystem::create_directories(projectRoot / "include");
        std::filesystem::create_directories(projectRoot / "ui");
        std::filesystem::create_directories(projectRoot / "generated");
        std::filesystem::create_directories(projectRoot / "cmake");

        const std::filesystem::path startupDocumentRelativePath =
            std::filesystem::path("ui") / normalizedStartupDocumentFileName;
        const std::filesystem::path startupDocumentPath =
            (projectRoot / startupDocumentRelativePath).lexically_normal();
        const std::filesystem::path titleBarDocumentRelativePath =
            std::filesystem::path("ui") / "TitleBar.ui.json";
        const std::filesystem::path titleBarDocumentPath =
            (projectRoot / titleBarDocumentRelativePath).lexically_normal();
        const std::string startupWidgetClassName =
            BuildStartupWidgetClassName(normalizedStartupDocumentFileName);
        std::shared_ptr<EditorSession> bootstrapSession = CreateSession();
        if (!bootstrapSession || !bootstrapSession->GetDocument()) {
            SetLocalizedOutputLine("NewProject.StartupSessionCreateFailed", "Create project failed: could not create startup document session.");
            return false;
        }

        bootstrapSession->GetDocument()->SetDisplayTitle(
            GetDocumentDisplayTitleFromFileName(normalizedStartupDocumentFileName));
        std::string documentError;
        if (!bootstrapSession->GetDocument()->SaveAs(startupDocumentPath, &documentError)) {
            SetLocalizedOutputLine("NewProject.CreateFailed", "Create project failed", ": " + documentError);
            return false;
        }

        EditorDocument titleBarDocument;
        titleBarDocument.NewDocument(BuildDefaultTitleBarRoot(trimmedProjectName), "TitleBar");
        if (!titleBarDocument.SaveAs(titleBarDocumentPath, &documentError)) {
            SetLocalizedOutputLine("NewProject.CreateFailed", "Create project failed", ": " + documentError);
            return false;
        }

        FProjectScaffoldRequest scaffoldRequest;
        scaffoldRequest.ProjectRoot = projectRoot;
        scaffoldRequest.ProjectName = trimmedProjectName;
        scaffoldRequest.NamespaceName = trimmedNamespaceName;
        scaffoldRequest.TemplateName = trimmedTemplateName;
        scaffoldRequest.StartupDocumentFileName = normalizedStartupDocumentFileName;
        scaffoldRequest.StartupWidgetClassName = startupWidgetClassName;
        scaffoldRequest.ApplicationSettings.Title = trimmedProjectName;
        scaffoldRequest.ApplicationSettings.bUseTitleBar = true;
        scaffoldRequest.ApplicationSettings.bShowSystemButtons = true;
        scaffoldRequest.ApplicationSettings.TitleBarDocumentRelativePath = titleBarDocumentRelativePath;
        scaffoldRequest.StartupRootWidget = bootstrapSession->GetDocument()->GetRootWidget();
        scaffoldRequest.TitleBarRootWidget = titleBarDocument.GetRootWidget();
        const FProjectScaffoldResult scaffoldResult = ProjectScaffolder::Scaffold(scaffoldRequest);
        if (!scaffoldResult.bSuccess) {
            SetLocalizedOutputLine("NewProject.CreateFailed", "Create project failed", ": " + scaffoldResult.ErrorMessage);
            return false;
        }

        auto project = std::make_shared<EditorProject>();
        if (!project->CreateNew(
                projectRoot,
                trimmedProjectName,
                trimmedNamespaceName,
                startupDocumentRelativePath,
                trimmedTemplateName)) {
            SetLocalizedOutputLine("NewProject.InvalidMetadata", "Create project failed: invalid project metadata.");
            return false;
        }
        FEditorApplicationSettings projectSettings = project->GetApplicationSettings();
        projectSettings.bUseTitleBar = true;
        projectSettings.bShowSystemButtons = true;
        projectSettings.TitleBarDocumentRelativePath = titleBarDocumentRelativePath;
        project->SetApplicationSettings(projectSettings);

        std::string manifestError;
        if (!project->Save(&manifestError)) {
            SetLocalizedOutputLine("NewProject.CreateFailed", "Create project failed", ": " + manifestError);
            return false;
        }

        ClearOpenDocuments();
        m_Project = project;
        m_ProjectRoot = projectRoot;
        RememberRecentFile(startupDocumentPath);

        std::shared_ptr<EditorSession> session = CreateSession();
        if (!session || !session->OpenDocumentFromPath(startupDocumentPath) || !AddSession(session, true)) {
            SetLocalizedOutputLine("Project.OpenedStartupLoadFailed", "Project created, but opening the startup document failed.");
            RebuildProjectView();
            NotifyProjectStateChanged();
            return false;
        }

        RebuildProjectView();
        NotifyProjectStateChanged();
        SetLocalizedOutputLine("NewProject.CreatedAppProject", "Created app project", " " + trimmedProjectName + " [" + trimmedTemplateName + "]");
        return true;
    } catch (const std::exception& error) {
        SetLocalizedOutputLine("NewProject.CreateFailed", "Create project failed", ": " + std::string(error.what()));
        return false;
    } catch (...) {
        SetLocalizedOutputLine("NewProject.CreateFailedWithPeriod", "Create project failed.");
        return false;
    }
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

    BeginBatchUiUpdate();
    if (m_Documents.empty()) {
        AddSession(CreateSession(), true);
    }
    if (!m_Documents.empty()) {
        ActivateDocumentTab(m_DocumentTabs ? m_DocumentTabs->GetActiveTabIndex() : 0);
    }
    EndBatchUiUpdate(true);
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
            SetLocalizedOutputLine(
                "Project.SwitchedToAlreadyOpenDocument",
                "Switched to already open document",
                ": " + openedDocument->GetFilePath().filename().string());
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

bool EditorWorkspaceController::NewAppProject(ImApplication& app)
{
    if (HasDirtyDocuments()) {
        SetLocalizedOutputLine("Project.CreateBlockedDirtyDocuments", "Create project blocked: save or discard dirty documents first.");
        return false;
    }

    FOpenFolderDialogOptions options;
    options.Title = EditorText("NewProject.SelectParentDirectory", "Select New Project Parent Directory").Resolve();
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Unsupported && m_OutputText) {
            m_OutputText->SetItems({EditorText("NewProject.Unsupported", "Create project is unsupported by the active platform backend.")});
        } else if (dialogResult.Code == EPathDialogResultCode::Error && m_OutputText) {
            m_OutputText->SetItems({FText::FromString(EditorText("NewProject.CreateFailed", "Create project failed").Resolve() + ": " + dialogResult.ErrorMessage)});
        }
        return false;
    }

    OpenCreateAppProjectDialog(app, dialogResult.Path);
    return true;
}

bool EditorWorkspaceController::OpenAppProject(ImApplication& app)
{
    if (HasDirtyDocuments()) {
        SetLocalizedOutputLine("Project.OpenBlockedDirtyDocuments", "Open project blocked: save or discard dirty documents first.");
        return false;
    }

    FOpenFolderDialogOptions options;
    options.Title = EditorText("Project.SelectAppProjectRoot", "Select App Project Root").Resolve();
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Unsupported && m_OutputText) {
            m_OutputText->SetItems({EditorText("Project.OpenUnsupported", "Open project is unsupported by the active platform backend.")});
        } else if (dialogResult.Code == EPathDialogResultCode::Error && m_OutputText) {
            m_OutputText->SetItems({FText::FromString(EditorText("Project.OpenFailed", "Open project failed").Resolve() + ": " + dialogResult.ErrorMessage)});
        }
        return false;
    }

    return OpenAppProjectAt(dialogResult.Path);
}

bool EditorWorkspaceController::OpenProjectSettings(ImApplication& app)
{
    if (!m_Project) {
        AppendOutputLine(EditorText("ProjectSettings.UnavailableNoActiveProject", "Project settings unavailable: no active project.").Resolve());
        return false;
    }

    OpenProjectSettingsDialog(app);
    return m_PendingProjectSettingsDialog != nullptr && m_PendingProjectSettingsDialog->IsOpen();
}

bool EditorWorkspaceController::OpenAppProjectAt(const std::filesystem::path& projectRoot)
{
    if (projectRoot.empty()) {
        return false;
    }

    if (HasDirtyDocuments()) {
        SetLocalizedOutputLine("Project.OpenBlockedDirtyDocuments", "Open project blocked: save or discard dirty documents first.");
        return false;
    }

    std::error_code errorCode;
    const std::filesystem::path normalizedRoot =
        std::filesystem::weakly_canonical(projectRoot, errorCode);
    const std::filesystem::path resolvedRoot =
        errorCode ? projectRoot.lexically_normal() : normalizedRoot;

    auto project = std::make_shared<EditorProject>();
    std::string manifestError;
    if (!project->Load(EditorProject::BuildManifestFilePath(resolvedRoot), &manifestError)) {
        if (m_OutputText) {
            m_OutputText->SetItems({FText::FromString(EditorText("Project.OpenFailed", "Open project failed").Resolve() + ": " + manifestError)});
        }
        return false;
    }

    ClearOpenDocuments();
    m_ProjectRoot = resolvedRoot;
    m_Project = project;

    const std::filesystem::path startupDocumentPath = project->GetStartupDocumentPath();
    if (!startupDocumentPath.empty() && std::filesystem::exists(startupDocumentPath)) {
        std::shared_ptr<EditorSession> session = CreateSession();
        if (!session || !session->OpenDocumentFromPath(startupDocumentPath) || !AddSession(session, true)) {
            if (m_OutputText) {
                m_OutputText->SetItems({EditorText("Project.OpenedStartupLoadFailed", "Project opened, but startup document could not be loaded.")});
            }
            RebuildProjectView();
            NotifyProjectStateChanged();
            return false;
        }

        RememberRecentFile(startupDocumentPath);
    } else {
        AddSession(CreateSession(), true);
    }

    RebuildProjectView();
    NotifyProjectStateChanged();
    if (m_OutputText) {
        m_OutputText->SetItems({FText::FromString(EditorText("Project.OpenedAppProject", "Opened app project").Resolve() + " " + project->GetProjectName())});
    }
    return true;
}

bool EditorWorkspaceController::ConfigureProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.ConfigureFailedProjectRootNotConfigured", "Configure failed: project root not configured.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Configure, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::ConfigureProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.ConfigureFailedProjectRootNotConfigured", "Configure failed: project root not configured.").Resolve());
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine(EditorText("Build.ConfigureFailedProfileNotFound", "Configure failed: build profile not found.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Configure, resolvedProfileName);
}

bool EditorWorkspaceController::BuildProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.BuildFailedProjectRootNotConfigured", "Build failed: project root not configured.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Build, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::BuildProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.BuildFailedProjectRootNotConfigured", "Build failed: project root not configured.").Resolve());
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine(EditorText("Build.BuildFailedProfileNotFound", "Build failed: build profile not found.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Build, resolvedProfileName);
}

bool EditorWorkspaceController::RunProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.RunFailedProjectRootNotConfigured", "Run failed: project root not configured.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Run, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::RunProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.RunFailedProjectRootNotConfigured", "Run failed: project root not configured.").Resolve());
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine(EditorText("Build.RunFailedProfileNotFound", "Run failed: build profile not found.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Run, resolvedProfileName);
}

bool EditorWorkspaceController::StopRunningProject()
{
    const std::shared_ptr<FBackgroundBuildTaskState> task = m_BackgroundBuildTask;
    if (!task) {
        return false;
    }

    std::shared_ptr<FProcessCancelToken> cancelToken;
    {
        std::lock_guard<std::mutex> lock(task->Mutex);
        if (task->Kind != EBackgroundBuildTaskKind::Run || task->bFinished || !task->ProcessCancelToken) {
            return false;
        }

        cancelToken = task->ProcessCancelToken;
        UpdateBackgroundBuildTaskStatus(task, EditorText("Build.StoppingRun", "Stopping run...").Resolve());
    }

    cancelToken->RequestCancel();
    AppendOutputLine(LocalizedEditorString("Build.StoppingRun", "Stopping run..."));
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::CleanProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.CleanFailedProjectRootNotConfigured", "Clean failed: project root not configured.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Clean, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::CleanProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.CleanFailedProjectRootNotConfigured", "Clean failed: project root not configured.").Resolve());
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine(EditorText("Build.CleanFailedProfileNotFound", "Clean failed: build profile not found.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Clean, resolvedProfileName);
}

bool EditorWorkspaceController::ClearBuildCache()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.ClearCacheFailedProjectRootNotConfigured", "Clear cache failed: project root not configured.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::ClearCache, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::ClearBuildCache(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.ClearCacheFailedProjectRootNotConfigured", "Clear cache failed: project root not configured.").Resolve());
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine(EditorText("Build.ClearCacheFailedProfileNotFound", "Clear cache failed: build profile not found.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::ClearCache, resolvedProfileName);
}

bool EditorWorkspaceController::RebuildProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.RebuildFailedProjectRootNotConfigured", "Rebuild failed: project root not configured.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Rebuild, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::RebuildProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine(EditorText("Build.RebuildFailedProjectRootNotConfigured", "Rebuild failed: project root not configured.").Resolve());
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine(EditorText("Build.RebuildFailedProfileNotFound", "Rebuild failed: build profile not found.").Resolve());
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Rebuild, resolvedProfileName);
}

void EditorWorkspaceController::TickBackgroundTasks()
{
    TickBackgroundBuildTask();
    TickBackgroundProbeTask();

    for (auto& document : m_Documents) {
        if (document.Session) {
            document.Session->TickDeferredRefreshes();
        }
    }
}

bool EditorWorkspaceController::IsBuildTaskRunning() const
{
    const std::shared_ptr<FBackgroundBuildTaskState> task = m_BackgroundBuildTask;
    if (!task) {
        return false;
    }

    std::lock_guard<std::mutex> lock(task->Mutex);
    return !task->bFinished;
}

bool EditorWorkspaceController::IsRunTaskRunning() const
{
    const std::shared_ptr<FBackgroundBuildTaskState> task = m_BackgroundBuildTask;
    if (!task) {
        return false;
    }

    std::lock_guard<std::mutex> lock(task->Mutex);
    return task->Kind == EBackgroundBuildTaskKind::Run && !task->bFinished;
}

std::string EditorWorkspaceController::GetBuildTaskStatusText() const
{
    const std::shared_ptr<FBackgroundBuildTaskState> task = m_BackgroundBuildTask;
    if (!task) {
        return std::string();
    }

    std::lock_guard<std::mutex> lock(task->Mutex);
    return task->StatusText;
}

std::string EditorWorkspaceController::GetActiveBuildProfileName() const
{
    return m_Project ? m_Project->GetActiveBuildProfileName() : std::string();
}

FEnvironmentProbeReport EditorWorkspaceController::GetActiveBuildProfileProbeReport() const
{
    FEnvironmentProbeReport report;
    if (!m_Project) {
        return report;
    }

    const FEditorBuildProfile* profile = m_Project->GetActiveBuildProfile();
    if (profile == nullptr) {
        return report;
    }

    report.TargetPlatform = profile->TargetPlatform;
    auto it = m_BuildProfileProbeReports.find(profile->Name);
    if (it != m_BuildProfileProbeReports.end()) {
        return it->second;
    }

    return report;
}

bool EditorWorkspaceController::TryGetBuildProfileProbeReport(
    const std::string& profileName,
    FEnvironmentProbeReport& outReport) const
{
    auto it = m_BuildProfileProbeReports.find(profileName);
    if (it == m_BuildProfileProbeReports.end()) {
        return false;
    }

    outReport = it->second;
    return true;
}

bool EditorWorkspaceController::IsBuildProfileProbeRefreshing(const std::string& profileName) const
{
    return !profileName.empty() &&
        m_RefreshingBuildProfileNames.find(profileName) != m_RefreshingBuildProfileNames.end();
}

bool EditorWorkspaceController::IsActiveBuildProfileProbeRefreshing() const
{
    return IsBuildProfileProbeRefreshing(GetActiveBuildProfileName());
}

void EditorWorkspaceController::RequestBuildProfileProbeRefresh()
{
    if (!m_Project) {
        InvalidateBuildProfileProbeCache();
        NotifyProjectStateChanged();
        return;
    }

    std::unordered_set<std::string> validProfileNames;
    for (const FEditorBuildProfile& profile : m_Project->GetBuildProfiles()) {
        validProfileNames.insert(profile.Name);
        m_RefreshingBuildProfileNames.insert(profile.Name);
    }

    for (auto it = m_BuildProfileProbeReports.begin(); it != m_BuildProfileProbeReports.end();) {
        if (validProfileNames.find(it->first) == validProfileNames.end()) {
            it = m_BuildProfileProbeReports.erase(it);
        } else {
            ++it;
        }
    }
    for (auto it = m_RefreshingBuildProfileNames.begin(); it != m_RefreshingBuildProfileNames.end();) {
        if (validProfileNames.find(*it) == validProfileNames.end()) {
            it = m_RefreshingBuildProfileNames.erase(it);
        } else {
            ++it;
        }
    }

    if (m_BackgroundProbeTask) {
        m_bBuildProfileProbeRefreshPending = true;
        m_bRefreshProjectViewOnProbeCompletion = true;
        NotifyProjectStateChanged();
        return;
    }

    m_bRefreshProjectViewOnProbeCompletion = true;
    StartBackgroundProbeTask();
    NotifyProjectStateChanged();
}

std::vector<std::string> EditorWorkspaceController::GetBuildProfileNames() const
{
    std::vector<std::string> names;
    if (!m_Project) {
        return names;
    }

    for (const FEditorBuildProfile& profile : m_Project->GetBuildProfiles()) {
        names.push_back(profile.Name);
    }
    return names;
}

bool EditorWorkspaceController::SetActiveBuildProfile(const std::string& profileName)
{
    if (!m_Project || !m_Project->SetActiveBuildProfileName(profileName)) {
        return false;
    }

    std::string saveError;
    if (!m_Project->Save(&saveError)) {
        AppendOutputLine(LocalizedEditorString(
            "Build.SaveActiveProfileFailed",
            "Failed to save active build profile",
            ": " + saveError));
    }
    if (m_BuildProfileProbeReports.find(profileName) == m_BuildProfileProbeReports.end()) {
        RequestBuildProfileProbeRefresh();
    }
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::UpdateBuildProfile(const FEditorBuildProfile& profile, bool bMakeActive)
{
    if (!m_Project) {
        return false;
    }

    FEditorBuildProfile* existingProfile = m_Project->FindBuildProfile(profile.Name);
    if (existingProfile == nullptr) {
        return false;
    }

    *existingProfile = profile;
    if (bMakeActive && !m_Project->SetActiveBuildProfileName(profile.Name)) {
        return false;
    }

    std::string saveError;
    if (!m_Project->Save(&saveError)) {
        AppendOutputLine(LocalizedEditorString(
            "Build.SaveProfileChangesFailed",
            "Failed to save build profile changes",
            ": " + saveError));
        return false;
    }

    RequestBuildProfileProbeRefresh();
    RebuildProjectView();
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::UpdateProjectSettings(
    const FEditorBuildProfile& profile,
    bool bMakeActive,
    const FEditorApplicationSettings& applicationSettings)
{
    if (!m_Project) {
        return false;
    }

    FEditorBuildProfile* existingProfile = m_Project->FindBuildProfile(profile.Name);
    if (existingProfile == nullptr) {
        return false;
    }

    *existingProfile = profile;
    if (bMakeActive && !m_Project->SetActiveBuildProfileName(profile.Name)) {
        return false;
    }

    m_Project->SetApplicationSettings(applicationSettings);
    std::string saveError;
    if (!m_Project->Save(&saveError)) {
        AppendOutputLine(LocalizedEditorString(
            "Build.SaveProfileChangesFailed",
            "Failed to save build profile changes",
            ": " + saveError));
        return false;
    }

    RequestBuildProfileProbeRefresh();
    RebuildProjectView();
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::RevealProjectBuildDirectory() const
{
    if (!m_Project || m_ProjectRoot.empty()) {
        return false;
    }

    const FEditorBuildProfile* activeProfile = m_Project->GetActiveBuildProfile();
    if (activeProfile == nullptr) {
        return false;
    }

    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(m_ProjectRoot, *activeProfile);
    if (!std::filesystem::exists(buildDirectory)) {
        return false;
    }

    return RevealProjectItemInExplorer(buildDirectory);
}

bool EditorWorkspaceController::RevealProjectBuildDirectory(const std::string& profileName) const
{
    if (!m_Project || m_ProjectRoot.empty()) {
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    const FEditorBuildProfile* profile = m_Project->FindBuildProfile(resolvedProfileName);
    if (profile == nullptr) {
        return false;
    }

    const std::filesystem::path buildDirectory = ResolveBuildDirectoryPath(m_ProjectRoot, *profile);
    if (!std::filesystem::exists(buildDirectory)) {
        return false;
    }

    return RevealProjectItemInExplorer(buildDirectory);
}

std::filesystem::path EditorWorkspaceController::ResolveExecutableDirectoryForConfiguration(
    const std::string& configuration) const
{
    if (!m_Project || m_ProjectRoot.empty() || configuration.empty()) {
        return {};
    }

    const FEditorBuildProfile* profile = FindWindowsBuildProfileForConfiguration(*m_Project, configuration);
    if (profile == nullptr) {
        return {};
    }

    return ResolveExecutableDirectoryPath(*m_Project, *profile);
}

bool EditorWorkspaceController::CanRevealExecutableDirectoryForConfiguration(
    const std::string& configuration) const
{
    const std::filesystem::path directoryPath = ResolveExecutableDirectoryForConfiguration(configuration);
    if (directoryPath.empty()) {
        return false;
    }

    std::error_code errorCode;
    return std::filesystem::is_directory(directoryPath, errorCode);
}

bool EditorWorkspaceController::RevealExecutableDirectoryForConfiguration(
    const std::string& configuration) const
{
    const std::filesystem::path directoryPath = ResolveExecutableDirectoryForConfiguration(configuration);
    if (directoryPath.empty()) {
        return false;
    }

    return RevealProjectItemInExplorer(directoryPath);
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

bool EditorWorkspaceController::RegenerateProjectCode()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        SetLocalizedOutputLine("Project.RegenerateCodeFailedNoProject", "Regenerate code failed: no app project is open.");
        return false;
    }

    if (m_Project->GetStartupDocumentRelativePath().empty()) {
        SetLocalizedOutputLine("Project.RegenerateCodeFailedNoStartupDocument", "Regenerate code failed: startup document is not configured.");
        return false;
    }

    const std::filesystem::path startupDocumentPath =
        (m_ProjectRoot / m_Project->GetStartupDocumentRelativePath()).lexically_normal();

    std::shared_ptr<EditorDocument> startupDocument;
    for (const FDocumentEntry& entry : m_Documents) {
        if (entry.Session &&
            entry.Session->GetDocument() &&
            entry.Session->GetDocument()->GetFilePath().lexically_normal() == startupDocumentPath) {
            startupDocument = entry.Session->GetDocument();
            break;
        }
    }

    if (!startupDocument) {
        startupDocument = std::make_shared<EditorDocument>();
        std::string loadError;
        if (!startupDocument->Load(startupDocumentPath, &loadError)) {
            SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": " + loadError);
            return false;
        }
    }

    if (!startupDocument->GetRootWidget()) {
        SetLocalizedOutputLine("Project.RegenerateCodeFailedNoRootWidget", "Regenerate code failed: startup document has no root widget.");
        return false;
    }

    FProjectScaffoldRequest scaffoldRequest;
    scaffoldRequest.ProjectRoot = m_ProjectRoot;
    scaffoldRequest.ProjectName = m_Project->GetProjectName();
    scaffoldRequest.NamespaceName = m_Project->GetNamespaceName();
    scaffoldRequest.TemplateName = m_Project->GetTemplateName();
    scaffoldRequest.StartupDocumentFileName = m_Project->GetStartupDocumentRelativePath().filename().string();
    scaffoldRequest.StartupWidgetClassName =
        BuildStartupWidgetClassName(scaffoldRequest.StartupDocumentFileName);
    scaffoldRequest.TitleBarWidgetClassName = "TitleBarView";
    scaffoldRequest.ApplicationSettings = m_Project->GetApplicationSettings();
    if (scaffoldRequest.ApplicationSettings.Title.empty()) {
        scaffoldRequest.ApplicationSettings.Title = scaffoldRequest.ProjectName;
    }
    scaffoldRequest.StartupRootWidget = startupDocument->GetRootWidget();

    if (scaffoldRequest.ApplicationSettings.bUseTitleBar) {
        if (scaffoldRequest.ApplicationSettings.TitleBarDocumentRelativePath.empty()) {
            scaffoldRequest.ApplicationSettings.TitleBarDocumentRelativePath =
                std::filesystem::path("ui") / "TitleBar.ui.json";
        }

        const std::filesystem::path titleBarDocumentPath =
            (m_ProjectRoot / scaffoldRequest.ApplicationSettings.TitleBarDocumentRelativePath).lexically_normal();
        std::shared_ptr<EditorDocument> titleBarDocument;
        for (const FDocumentEntry& entry : m_Documents) {
            if (entry.Session &&
                entry.Session->GetDocument() &&
                entry.Session->GetDocument()->GetFilePath().lexically_normal() == titleBarDocumentPath) {
                titleBarDocument = entry.Session->GetDocument();
                break;
            }
        }

        if (!titleBarDocument) {
            titleBarDocument = std::make_shared<EditorDocument>();
            std::string loadError;
            if (!std::filesystem::exists(titleBarDocumentPath)) {
                titleBarDocument->NewDocument(BuildDefaultTitleBarRoot(scaffoldRequest.ProjectName), "TitleBar");
                std::string saveError;
                if (!titleBarDocument->SaveAs(titleBarDocumentPath, &saveError)) {
                    SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": " + saveError);
                    return false;
                }

                FEditorApplicationSettings updatedSettings = m_Project->GetApplicationSettings();
                updatedSettings.TitleBarDocumentRelativePath = scaffoldRequest.ApplicationSettings.TitleBarDocumentRelativePath;
                m_Project->SetApplicationSettings(updatedSettings);
                std::string projectSaveError;
                if (!m_Project->Save(&projectSaveError)) {
                    SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": " + projectSaveError);
                    return false;
                }
            } else if (!titleBarDocument->Load(titleBarDocumentPath, &loadError)) {
                SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": " + loadError);
                return false;
            }
        }

        if (!std::dynamic_pointer_cast<ImTitleBar>(titleBarDocument->GetRootWidget())) {
            SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": title bar document root must be ImTitleBar.");
            return false;
        }

        if (auto titleBarRoot = std::dynamic_pointer_cast<ImTitleBar>(titleBarDocument->GetRootWidget())) {
            titleBarRoot->SetShowSystemButtons(scaffoldRequest.ApplicationSettings.bShowSystemButtons);
            std::string saveError;
            if (titleBarRoot->GetShowSystemButtons() != scaffoldRequest.ApplicationSettings.bShowSystemButtons ||
                (titleBarDocument->HasFilePath() && !titleBarDocument->Save(&saveError))) {
                SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": " + saveError);
                return false;
            }
        }
        scaffoldRequest.TitleBarRootWidget = titleBarDocument->GetRootWidget();
    }

    const FProjectScaffoldResult result = ProjectScaffolder::GenerateCode(scaffoldRequest);
    if (!result.bSuccess) {
        SetLocalizedOutputLine("Project.RegenerateCodeFailed", "Regenerate code failed", ": " + result.ErrorMessage);
        return false;
    }

    if (scaffoldRequest.TitleBarRootWidget) {
        EnsureGeneratedSourceInCMakeLists(scaffoldRequest.TitleBarWidgetClassName + ".cpp");
    }

    SetLocalizedOutputLine("Project.RegeneratedCode", "Regenerated project code.");
    RefreshProjectTree();
    return true;
}

bool EditorWorkspaceController::CloseActiveDocument(ImApplication& app)
{
    return CloseDocumentAt(app, m_ActiveDocumentIndex);
}

bool EditorWorkspaceController::EnsureGeneratedSourceInCMakeLists(const std::string& generatedSourceFileName)
{
    if (m_ProjectRoot.empty() || generatedSourceFileName.empty()) {
        return false;
    }

    const std::filesystem::path cmakeListsPath = m_ProjectRoot / "CMakeLists.txt";
    std::ifstream input(cmakeListsPath, std::ios::binary);
    if (!input.is_open()) {
        return false;
    }

    std::stringstream buffer;
    buffer << input.rdbuf();
    std::string text = buffer.str();
    const std::string sourceLine = "    generated/" + generatedSourceFileName;
    if (text.find(sourceLine) != std::string::npos) {
        return true;
    }

    const std::size_t appSourcesPos = text.find("set(IMWIDGETV4_APP_SOURCES");
    if (appSourcesPos == std::string::npos) {
        return false;
    }

    const std::size_t listEnd = text.find(")\n", appSourcesPos);
    if (listEnd == std::string::npos) {
        return false;
    }

    text.insert(listEnd, sourceLine + "\n");
    std::ofstream output(cmakeListsPath, std::ios::binary | std::ios::trunc);
    if (!output.is_open()) {
        return false;
    }

    output << text;
    output.flush();
    return output.good();
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

std::vector<std::string> EditorWorkspaceController::GetOutputLines() const
{
    return m_OutputText ? m_OutputText->GetItems() : std::vector<std::string>();
}

std::shared_ptr<EditorSession> EditorWorkspaceController::CreateSession() const
{
    return std::make_shared<EditorSession>(m_CreateDefaultDocumentRoot);
}

EditorWorkspaceController::FSessionWidgets EditorWorkspaceController::CreateSessionWidgets()
{
    FSessionWidgets widgets;

    widgets.DocumentHost = CreateDocumentHost();
    widgets.PreviewHost = CreatePreviewHost();
    widgets.SchemaText = CreateSchemaText();
    widgets.HeaderPreviewText = CreateCodePreviewText();
    widgets.SourcePreviewText = CreateCodePreviewText();
    widgets.DesignerSurface = std::make_shared<ImDesignerSurface>();
    widgets.DesignerHost = std::make_shared<EditorDesignerSurfaceHost>();
    widgets.DesignerHost->SetWorkspaceController(shared_from_this());
    widgets.DesignerHost->SetDesignerSurface(widgets.DesignerSurface);
    widgets.DocumentHost->SetContent(widgets.DesignerHost);

    widgets.WorkspaceTabs = std::make_shared<ImTabView>();
    widgets.WorkspaceTabs->SetSupportsKeyboardFocus(true);
    widgets.WorkspaceTabs->SetStyle(CreateWorkspaceTabStyle());
    widgets.WorkspaceTabs->AddTab(EditorText("Workspace.Designer", "Designer"), widgets.DocumentHost);
    widgets.WorkspaceTabs->AddTab(EditorText("Workspace.Preview", "Preview"), widgets.PreviewHost);
    widgets.WorkspaceTabs->AddTab(EditorText("Workspace.Schema", "Schema"), widgets.SchemaText);
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

    RequestProjectViewRefresh();
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
    if (entry.Widgets.DesignerHost) {
        entry.Widgets.DesignerHost->SetWorkspaceController(shared_from_this());
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
        entry.Widgets.SourcePreviewText,
        entry.Widgets.WorkspaceTabs);

    RequestProjectViewRefresh();
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
            m_OutputText->SetItems({EditorText("Output.NoOpenDocuments", "No open documents.")});
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
    popupMenu->SetStyle(MakeEditorPopupMenuStyle(popupMenu->GetStyle()));

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
    popupOptions.Style = MakeEditorPopupWindowStyle();

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
    popupMenu->SetStyle(MakeEditorPopupMenuStyle(popupMenu->GetStyle()));

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
    popupOptions.Style = MakeEditorPopupWindowStyle();

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
    popupMenu->SetStyle(MakeEditorPopupMenuStyle(popupMenu->GetStyle()));

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(MakeEditorMenuItem(
        "Menu.SaveAllAndSwitch",
        "Save All and Switch",
        true,
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
                self->SetLocalizedOutputLine("Workspace.ProjectRoot", "Project root", ": " + pendingProjectRoot.string());
                self->ClosePendingPrompt();
            }
        }));
    items.push_back(MakeEditorMenuItem(
        "Menu.DiscardChangesAndSwitch",
        "Discard Changes and Switch",
        true,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                const std::filesystem::path pendingProjectRoot = self->m_PendingProjectRootChange;
                self->m_PendingProjectRootChange.clear();
                self->SetProjectRoot(pendingProjectRoot);
                self->SetLocalizedOutputLine("Workspace.ProjectRoot", "Project root", ": " + pendingProjectRoot.string());
                self->ClosePendingPrompt();
            }
        }));
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(MakeEditorMenuItem(
        "Common.Cancel",
        "Cancel",
        true,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->m_PendingProjectRootChange.clear();
                self->ClosePendingPrompt();
            }
        }));
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
    popupOptions.Style = MakeEditorPopupWindowStyle();

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
    dialogOptions.HeadingText = std::filesystem::is_directory(path)
        ? EditorText("Project.RenameFolder", "Rename Folder").Resolve()
        : EditorText("Project.RenameFile", "Rename File").Resolve();
    dialogOptions.InitialText = path.filename().string();
    dialogOptions.ConfirmText = EditorText("Project.Rename", "Rename").Resolve();
    dialogOptions.CancelText = EditorText("Common.Cancel", "Cancel").Resolve();
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
    dialogOptions.HeadingText = EditorText("Project.CreateUIDocument", "Create UI Document").Resolve();
    dialogOptions.InitialText = defaultDocumentPath.empty()
        ? std::string("NewWidget.ui.json")
        : defaultDocumentPath.filename().string();
    dialogOptions.ConfirmText = EditorText("Project.Create", "Create").Resolve();
    dialogOptions.CancelText = EditorText("Common.Cancel", "Cancel").Resolve();
    dialogOptions.Size = FVector2(400.0f, 116.0f);
    dialogOptions.bSelectAllOnOpen = true;
    dialogOptions.OnConfirm = [weakThis, directoryPath](const std::string& fileName) {
        if (auto self = weakThis.lock()) {
            const std::filesystem::path trimmedName = std::filesystem::path(fileName).filename();
            if (trimmedName.empty() || trimmedName != fileName) {
                self->SetLocalizedOutputLine("Project.CreateDocumentFileNamePathSeparator", "Create document failed: file name must not contain path separators.");
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

void EditorWorkspaceController::OpenCreateAppProjectDialog(
    ImApplication& app,
    const std::filesystem::path& parentDirectory)
{
    ClosePendingPrompt();
    CloseDocumentTabContextMenu();
    CloseProjectItemContextMenu();
    m_PendingCreateProjectParentPath = parentDirectory;

    auto weakThis = weak_from_this();
    const std::filesystem::path defaultProjectPath =
        BuildUniqueChildPath(parentDirectory, "NewAppProject");
    FNewAppProjectDialogOptions dialogOptions;
    dialogOptions.PopupTitle = "CreateAppProjectDialog";
    dialogOptions.HeadingText = EditorText("NewProject.CreateAppProject", "Create App Project").Resolve();
    dialogOptions.ParentDirectory = parentDirectory;
    dialogOptions.InitialOptions.ProjectName = defaultProjectPath.empty()
        ? std::string("NewAppProject")
        : defaultProjectPath.filename().string();
    dialogOptions.InitialOptions.NamespaceName =
        NormalizeProjectIdentifier(dialogOptions.InitialOptions.ProjectName, "AppProject");
    dialogOptions.InitialOptions.StartupDocumentName = "Main";
    dialogOptions.InitialOptions.TemplateName = "Blank App";
    dialogOptions.TemplateOptions = GetAvailableProjectTemplateNames();
    dialogOptions.InitialTemplateIndex = 0;
    dialogOptions.ConfirmText = EditorText("Project.Create", "Create").Resolve();
    dialogOptions.CancelText = EditorText("Common.Cancel", "Cancel").Resolve();
    dialogOptions.Size = FVector2(520.0f, 316.0f);
    dialogOptions.OnConfirm = [weakThis, parentDirectory](const FCreateAppProjectOptions& options) {
        if (auto self = weakThis.lock()) {
            if (self->CreateAppProjectAt(parentDirectory, options)) {
                self->m_PendingCreateProjectParentPath.clear();
                return true;
            }
        }
        return false;
    };
    dialogOptions.OnCancel = [weakThis]() {
        if (auto self = weakThis.lock()) {
            self->m_PendingCreateProjectParentPath.clear();
        }
    };
    dialogOptions.Position = FVector2(220.0f, 120.0f);
    if (m_ProjectView) {
        const FGeometry geometry = m_ProjectView->GetGeometry();
        dialogOptions.Position = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.30f),
            geometry.Position.Y + 52.0f);
    }

    m_PendingNewAppProjectDialog = std::make_shared<NewAppProjectDialog>();
    m_PendingNewAppProjectDialog->Open(app, dialogOptions);
    m_CloseConfirmWindow = m_PendingNewAppProjectDialog->GetWindow();
    m_CloseConfirmMenu.reset();
}

void EditorWorkspaceController::OpenProjectSettingsDialog(ImApplication& app)
{
    if (!m_Project) {
        return;
    }

    FProjectSettingsDialogOptions dialogOptions;
    dialogOptions.ProjectName = m_Project->GetProjectName();
    dialogOptions.NamespaceName = m_Project->GetNamespaceName();
    dialogOptions.StartupDocument = m_Project->GetStartupDocumentRelativePath().generic_string();
    dialogOptions.ApplicationSettings = m_Project->GetApplicationSettings();
    dialogOptions.BuildProfiles = m_Project->GetBuildProfiles();
    dialogOptions.ActiveBuildProfileName = m_Project->GetActiveBuildProfileName();
    dialogOptions.OnConfirm = [weakThis = weak_from_this()](
        const FEditorBuildProfile& profile,
        bool bMakeActive,
        const FEditorApplicationSettings& applicationSettings) {
        if (auto self = weakThis.lock()) {
            return self->UpdateProjectSettings(profile, bMakeActive, applicationSettings);
        }
        return false;
    };
    dialogOptions.Position = FVector2(240.0f, 120.0f);
    if (m_ProjectView) {
        const FGeometry geometry = m_ProjectView->GetGeometry();
        dialogOptions.Position = FVector2(
            geometry.Position.X + std::max(24.0f, geometry.Size.X * 0.25f),
            geometry.Position.Y + 42.0f);
    }

    m_PendingProjectSettingsDialog = std::make_shared<ProjectSettingsDialog>();
    m_PendingProjectSettingsDialog->Open(app, dialogOptions);
    m_CloseConfirmWindow = m_PendingProjectSettingsDialog->GetWindow();
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
    dialogOptions.HeadingText = EditorText("Project.CreateFolder", "Create Folder").Resolve();
    dialogOptions.InitialText = defaultFolderPath.empty()
        ? std::string("NewFolder")
        : defaultFolderPath.filename().string();
    dialogOptions.ConfirmText = EditorText("Project.Create", "Create").Resolve();
    dialogOptions.CancelText = EditorText("Common.Cancel", "Cancel").Resolve();
    dialogOptions.Size = FVector2(360.0f, 116.0f);
    dialogOptions.bSelectAllOnOpen = true;
    dialogOptions.OnConfirm = [weakThis, directoryPath](const std::string& folderName) {
        if (auto self = weakThis.lock()) {
            const std::filesystem::path trimmedName = std::filesystem::path(folderName).filename();
            if (trimmedName.empty() || trimmedName != folderName) {
                self->SetLocalizedOutputLine("Project.CreateFolderNamePathSeparator", "Create folder failed: folder name must not contain path separators.");
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
    popupMenu->SetStyle(MakeEditorPopupMenuStyle(popupMenu->GetStyle()));

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(MakeEditorMenuItem(
        std::filesystem::is_directory(path) ? "Menu.DeleteFolder" : "Menu.DeleteFile",
        std::filesystem::is_directory(path) ? "Delete Folder" : "Delete File",
        true,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                const std::filesystem::path pendingPath = self->m_PendingDeleteProjectItemPath;
                self->m_PendingDeleteProjectItemPath.clear();
                self->DeleteProjectItem(pendingPath);
                self->ClosePendingPrompt();
            }
        }));
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(MakeEditorMenuItem(
        "Common.Cancel",
        "Cancel",
        true,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->m_PendingDeleteProjectItemPath.clear();
                self->ClosePendingPrompt();
            }
        }));
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
    popupOptions.Style = MakeEditorPopupWindowStyle();

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
    m_PendingNewAppProjectDialog.reset();
    m_PendingProjectSettingsDialog.reset();
    m_PendingCloseDocumentIndex = -1;
    m_PendingProjectRootChange.clear();
    m_PendingCreateProjectParentPath.clear();
    m_PendingRenameProjectItemPath.clear();
    m_PendingDeleteProjectItemPath.clear();
}

EditorWorkspaceController::json EditorWorkspaceController::BuildWorkspaceStateJson() const
{
    json workspaceState;
    workspaceState["Format"] = "ImWidgetV4EditorWorkspaceState";
    workspaceState["Version"] = 1;
    workspaceState["ProjectRoot"] = m_ProjectRoot.string();
    workspaceState["ProjectManifest"] = m_Project
        ? m_Project->GetManifestFilePath().string()
        : std::string();
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

    BeginBatchUiUpdate();

    const std::string projectRoot = workspaceState.value("ProjectRoot", std::string());
    if (!projectRoot.empty()) {
        SetProjectRoot(projectRoot);
    }
    const std::string projectManifestPath = workspaceState.value("ProjectManifest", std::string());
    if (!projectManifestPath.empty()) {
        LoadProjectManifestAtRoot(std::filesystem::path(projectManifestPath).parent_path(), false);
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

    EndBatchUiUpdate(true);
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
    popupMenu->SetStyle(MakeEditorPopupMenuStyle(popupMenu->GetStyle()));

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;
    items.push_back(MakeEditorMenuItem(
        "Menu.Activate",
        "Activate",
        true,
        [weakThis, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                self->CloseDocumentTabContextMenu();
            }
        }));
    items.push_back(MakeEditorMenuItem(
        "Menu.Save",
        "Save",
        true,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->SaveDocument(*application);
                }
                self->CloseDocumentTabContextMenu();
            }
        }));
    items.push_back(MakeEditorMenuItem(
        "Menu.SaveAs",
        "Save As...",
        true,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->SaveDocumentAs(*application);
                }
                self->CloseDocumentTabContextMenu();
            }
        }));
    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(MakeEditorMenuItem(
        "Common.Close",
        "Close",
        true,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->CloseDocumentAt(*application, index);
                }
                self->CloseDocumentTabContextMenu();
            }
        }));
    items.push_back(MakeEditorMenuItem(
        "Menu.CloseOthers",
        "Close Others",
        true,
        [weakThis, application = &app, index]() {
            if (auto self = weakThis.lock()) {
                self->ActivateDocumentAt(index);
                if (application) {
                    self->CloseOtherDocuments(*application, index);
                }
                self->CloseDocumentTabContextMenu();
            }
        }));
    items.push_back(MakeEditorMenuItem(
        "Menu.CloseAll",
        "Close All",
        true,
        [weakThis, application = &app]() {
            if (auto self = weakThis.lock()) {
                if (application) {
                    self->CloseAllDocuments(*application);
                }
                self->CloseDocumentTabContextMenu();
            }
        }));
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
    popupOptions.Style = MakeEditorPopupWindowStyle();

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
    popupMenu->SetStyle(MakeEditorPopupMenuStyle(popupMenu->GetStyle()));

    auto weakThis = weak_from_this();
    std::vector<FPopupMenuItem> items;

    if (binding.Kind == EProjectItemKind::OpenDocument) {
        items.push_back(MakeEditorMenuItem(
            "Menu.Activate",
            "Activate",
            true,
            [weakThis, index = binding.Index]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    self->CloseProjectItemContextMenu();
                }
            }));

        items.push_back(MakeEditorMenuItem(
            "Menu.Save",
            "Save",
            true,
            [weakThis, index = binding.Index, application = &app]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    if (application) {
                        self->SaveDocument(*application);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));

        items.push_back(MakeEditorMenuItem(
            "Menu.SaveAs",
            "Save As...",
            true,
            [weakThis, index = binding.Index, application = &app]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    if (application) {
                        self->SaveDocumentAs(*application);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));

        items.push_back(MakeEditorMenuItem(
            "Common.Close",
            "Close",
            true,
            [weakThis, index = binding.Index, application = &app]() {
                if (auto self = weakThis.lock()) {
                    self->ActivateDocumentAt(index);
                    if (application) {
                        self->CloseDocumentAt(*application, index);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));
    }

    if (binding.Kind == EProjectItemKind::RecentFile ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        items.push_back(MakeEditorMenuItem(
            "Menu.Open",
            "Open",
            true,
            [weakThis, path = binding.Path]() {
                if (auto self = weakThis.lock()) {
                    self->OpenDocumentFromPath(path);
                    self->CloseProjectItemContextMenu();
                }
            }));
    }

    if (binding.Kind == EProjectItemKind::BuildProfile) {
        const bool bBuildRunning = IsBuildTaskRunning();
        const bool bIsActiveProfile = m_Project && binding.ProfileName == m_Project->GetActiveBuildProfileName();

        items.push_back(MakeEditorMenuItem(
            bIsActiveProfile
                ? EditorText("Menu.ActiveProfile", "Active Profile")
                : EditorText("Menu.SetActiveProfile", "Set Active Profile"),
            !bBuildRunning && !binding.ProfileName.empty(),
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->SetActiveBuildProfile(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }));
        items.push_back(MakeEditorMenuItem(
            "Menu.ConfigureThisProfile",
            "Configure This Profile",
            !bBuildRunning && !binding.ProfileName.empty(),
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->ConfigureProject(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }));
        items.push_back(MakeEditorMenuItem(
            "Menu.BuildThisProfile",
            "Build This Profile",
            !bBuildRunning && !binding.ProfileName.empty(),
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->BuildProject(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }));
        items.push_back(MakeEditorMenuItem(
            "Menu.RevealBuildFolder",
            "Reveal Build Folder",
            !binding.ProfileName.empty(),
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->RevealProjectBuildDirectory(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }));
        items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
        items.push_back(MakeEditorMenuItem(
            "Menu.ProjectSettings",
            "Project Settings...",
            true,
            [weakThis, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->OpenProjectSettings(*application);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        items.push_back(MakeEditorMenuItem(
            "Menu.SetAsProjectRoot",
            "Set As Project Root",
            true,
            [weakThis, path = binding.Path, kind = binding.Kind, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->RequestProjectRootChange(
                            *application,
                            kind == EProjectItemKind::WorkspaceDirectory ? path : path.parent_path());
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory) {
        items.push_back(MakeEditorMenuItem(
            "Menu.NewUIDocument",
            "New UI Document...",
            true,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->CreateDocumentInDirectory(*application, path);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));

        items.push_back(MakeEditorMenuItem(
            "Menu.NewFolder",
            "New Folder",
            true,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->CreateFolderInDirectory(*application, path);
                    }
                    self->CloseProjectItemContextMenu();
                }
            }));

        items.push_back(MakeEditorMenuItem(
            "Menu.RefreshThisFolder",
            "Refresh This Folder",
            true,
            [weakThis]() {
                if (auto self = weakThis.lock()) {
                    self->RefreshProjectTree();
                    self->CloseProjectItemContextMenu();
                }
            }));
    }

    if (binding.Kind == EProjectItemKind::WorkspaceDirectory ||
        binding.Kind == EProjectItemKind::WorkspaceFile) {
        items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
        items.push_back(MakeEditorMenuItem(
            "Menu.Rename",
            "Rename...",
            true,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->OpenRenameProjectItemDialog(*application, path);
                    }
                }
            }));
        items.push_back(MakeEditorMenuItem(
            "Menu.RevealInFileManager",
            "Reveal in File Manager",
            true,
            [weakThis, path = binding.Path]() {
                if (auto self = weakThis.lock()) {
                    self->RevealProjectItemInExplorer(path);
                    self->CloseProjectItemContextMenu();
                }
            }));
        items.push_back(MakeEditorMenuItem(
            "Menu.Delete",
            "Delete",
            true,
            [weakThis, path = binding.Path, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->PromptDeleteProjectItem(*application, path);
                    }
                }
            }));
    }

    items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
    items.push_back(MakeEditorMenuItem(
        "Menu.RefreshProjectTree",
        "Refresh Project Tree",
        true,
        [weakThis]() {
            if (auto self = weakThis.lock()) {
                self->RefreshProjectTree();
                self->CloseProjectItemContextMenu();
            }
        }));

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
    popupOptions.Style = MakeEditorPopupWindowStyle();

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
        SetLocalizedOutputLine("Project.CreateDocumentFailed", "Create document failed", ": " + errorMessage);
        return false;
    }

    RememberRecentFile(normalizedPath);
    const bool bAdded = AddSession(session, true);
    if (bAdded) {
        SetLocalizedOutputLine("Project.Created", "Created", " " + normalizedPath.filename().string());
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
            SetLocalizedOutputLine(
                "Project.CreateFolderSkippedAlreadyExists",
                "Create folder skipped",
                ": " + directoryPath.filename().string() + " " +
                    EditorText("Project.AlreadyExists", "already exists.").Resolve());
            return false;
        }

        const std::filesystem::path parentPath = directoryPath.parent_path();
        if (!parentPath.empty()) {
            std::filesystem::create_directories(parentPath);
        }

        if (!std::filesystem::create_directory(directoryPath)) {
            SetLocalizedOutputLine("Project.CreateFolderUnableToCreate", "Create folder failed: unable to create", " " + directoryPath.filename().string());
            return false;
        }
    } catch (const std::exception& exception) {
        SetLocalizedOutputLine("Project.CreateFolderFailed", "Create folder failed", ": " + std::string(exception.what()));
        return false;
    } catch (...) {
        SetLocalizedOutputLine("Project.CreateFolderFailedWithPeriod", "Create folder failed.");
        return false;
    }

    RefreshProjectTree();
    SetLocalizedOutputLine("Project.CreatedFolder", "Created folder", " " + directoryPath.filename().string());
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
        SetLocalizedOutputLine("Project.RenameNamePathSeparator", "Rename failed: name must not contain path separators.");
        return false;
    }

    const std::filesystem::path targetPath = path.parent_path() / trimmedName;
    if (AreEquivalentPaths(targetPath, path)) {
        return true;
    }

    if (std::filesystem::exists(targetPath)) {
        SetLocalizedOutputLine("Project.RenameTargetExists", "Rename failed: target already exists.");
        return false;
    }

    for (const FDocumentEntry& entry : m_Documents) {
        if (!entry.Session || !entry.Session->GetDocument() || !entry.Session->GetDocument()->HasFilePath()) {
            continue;
        }

        const std::filesystem::path documentPath = entry.Session->GetDocument()->GetFilePath();
        if ((AreEquivalentPaths(documentPath, path) || IsPathWithinRoot(documentPath, path)) &&
            entry.Session->GetDocument()->IsDirty()) {
            SetLocalizedOutputLine("Project.RenameBlockedDirtyDocuments", "Rename blocked: save or discard dirty documents under the selected path first.");
            return false;
        }
    }

    try {
        std::filesystem::rename(path, targetPath);
    } catch (const std::exception& exception) {
        SetLocalizedOutputLine("Project.RenameFailed", "Rename failed", ": " + std::string(exception.what()));
        return false;
    } catch (...) {
        SetLocalizedOutputLine("Project.RenameFailedWithPeriod", "Rename failed.");
        return false;
    }

    ReplaceRecentFilePath(path, targetPath);
    UpdateOpenDocumentPathsForRename(path, targetPath);
    if (!m_ProjectRoot.empty() && AreEquivalentPaths(m_ProjectRoot, path)) {
        m_ProjectRoot = targetPath;
        LoadProjectManifestAtRoot(m_ProjectRoot, false);
    }

    RefreshProjectTree();
    SetLocalizedOutputLine(
        "Project.Renamed",
        "Renamed",
        " " + path.filename().string() + " " + EditorText("Project.To", "to").Resolve() + " " + targetPath.filename().string());
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
        if (bBlockedByDirtyDocument) {
            SetLocalizedOutputLine("Project.DeleteBlockedDirtyDocuments", "Delete blocked: save or discard dirty documents under the selected path first.");
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
        SetLocalizedOutputLine("Project.DeleteFailed", "Delete failed", ": " + std::string(exception.what()));
        return false;
    } catch (...) {
        SetLocalizedOutputLine("Project.DeleteFailedWithPeriod", "Delete failed.");
        return false;
    }

    RefreshProjectTree();
    SetLocalizedOutputLine("Project.Deleted", "Deleted", " " + path.filename().string());
    NotifyProjectStateChanged();
    return true;
}

bool EditorWorkspaceController::RevealProjectItemInExplorer(const std::filesystem::path& path) const
{
    if (path.empty() || !std::filesystem::exists(path)) {
        return false;
    }

    if (!m_ShellHost) {
        return false;
    }

    ImApplication* application = m_ShellHost->GetApplication();
    if (application == nullptr) {
        return false;
    }

    const bool bSelectItemIfPossible = !std::filesystem::is_directory(path);
    return application->RevealPathInFileManager(path, bSelectItemIfPossible);
}

void EditorWorkspaceController::NotifyProjectStateChanged() const
{
    EditorWorkspaceController* self = const_cast<EditorWorkspaceController*>(this);
    if (self->m_bBatchUiUpdateActive) {
        self->m_bProjectStateNotificationPending = true;
        return;
    }

    if (m_OnProjectStateChanged) {
        m_OnProjectStateChanged();
    }
}

void EditorWorkspaceController::SetOutputLine(const std::string& text) const
{
    if (!m_OutputText) {
        return;
    }

    m_OutputText->SetItems({text});
}

void EditorWorkspaceController::SetLocalizedOutputLine(
    const std::string& key,
    const std::string& defaultText,
    const std::string& suffix) const
{
    SetOutputLine(LocalizedEditorString(key, defaultText, suffix));
}

void EditorWorkspaceController::AppendOutputLine(const std::string& text) const
{
    if (!m_OutputText || text.empty()) {
        return;
    }

    std::vector<std::string> items = m_OutputText->GetItems();
    items.push_back(text);
    constexpr std::size_t kMaxOutputLines = 300;
    if (items.size() > kMaxOutputLines) {
        items.erase(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(items.size() - kMaxOutputLines));
    }

    m_OutputText->SetItems(items);
    if (!items.empty()) {
        m_OutputText->ScrollToItem(static_cast<int>(items.size() - 1), false);
    }
}

void EditorWorkspaceController::InvalidateBuildProfileProbeCache()
{
    ShutdownBackgroundProbeTask();
    m_BuildProfileProbeReports.clear();
    m_RefreshingBuildProfileNames.clear();
    m_bBuildProfileProbeRefreshPending = false;
    m_bRefreshProjectViewOnProbeCompletion = false;
}

void EditorWorkspaceController::StartBackgroundProbeTask()
{
    if (!m_Project) {
        return;
    }

    const std::vector<FEditorBuildProfile> profiles = m_Project->GetBuildProfiles();
    if (profiles.empty()) {
        m_RefreshingBuildProfileNames.clear();
        m_BuildProfileProbeReports.clear();
        return;
    }

    auto task = std::make_shared<FBackgroundProbeTaskState>();
    m_BackgroundProbeTask = task;
    task->Worker = std::thread([task, profiles]() {
        std::unordered_map<std::string, FEnvironmentProbeReport> results;
        results.reserve(profiles.size());
        for (const FEditorBuildProfile& profile : profiles) {
            results[profile.Name] = EnvironmentProbe::Probe(profile);
        }

        std::lock_guard<std::mutex> lock(task->Mutex);
        task->Results = std::move(results);
        task->bFinished = true;
    });
}

void EditorWorkspaceController::TickBackgroundProbeTask()
{
    const std::shared_ptr<FBackgroundProbeTaskState> task = m_BackgroundProbeTask;
    if (!task) {
        return;
    }

    bool bFinished = false;
    std::unordered_map<std::string, FEnvironmentProbeReport> results;
    {
        std::lock_guard<std::mutex> lock(task->Mutex);
        bFinished = task->bFinished;
        if (bFinished) {
            results = task->Results;
        }
    }

    if (!bFinished) {
        return;
    }

    if (task->Worker.joinable()) {
        task->Worker.join();
    }

    m_BackgroundProbeTask.reset();
    m_BuildProfileProbeReports = std::move(results);
    m_RefreshingBuildProfileNames.clear();

    const bool bRefreshProjectView = m_bRefreshProjectViewOnProbeCompletion;
    m_bRefreshProjectViewOnProbeCompletion = false;
    if (bRefreshProjectView) {
        RebuildProjectView();
    }

    NotifyProjectStateChanged();

    if (m_bBuildProfileProbeRefreshPending) {
        m_bBuildProfileProbeRefreshPending = false;
        RequestBuildProfileProbeRefresh();
    }
}

void EditorWorkspaceController::ShutdownBackgroundProbeTask()
{
    const std::shared_ptr<FBackgroundProbeTaskState> task = m_BackgroundProbeTask;
    if (!task) {
        return;
    }

    if (task->Worker.joinable()) {
        task->Worker.join();
    }

    m_BackgroundProbeTask.reset();
}

bool EditorWorkspaceController::StartBackgroundBuildTask(
    EBackgroundBuildTaskKind kind,
    const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        return false;
    }

    if (IsBuildTaskRunning()) {
        AppendOutputLine(LocalizedEditorString("Build.RequestIgnoredTaskRunning", "Build request ignored: another background build task is already running."));
        return false;
    }

    ShutdownBackgroundBuildTask();

    std::shared_ptr<EditorProject> project = m_Project;
    if (!project) {
        return false;
    }

    auto task = std::make_shared<FBackgroundBuildTaskState>();
    task->Kind = kind;
    task->ProfileName = profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (kind == EBackgroundBuildTaskKind::Run) {
        task->ProcessCancelToken = std::make_shared<FProcessCancelToken>();
    }
    const FEditorBuildProfile* selectedProfile = project->FindBuildProfile(task->ProfileName);
    if (task->ProfileName.empty() || selectedProfile == nullptr) {
        AppendOutputLine(LocalizedEditorString("Build.RequestIgnoredProfileNotFound", "Build request ignored: build profile not found."));
        m_BackgroundBuildTask.reset();
        return false;
    }

    if (kind == EBackgroundBuildTaskKind::Run &&
        selectedProfile->TargetPlatform != EEditorTargetPlatform::WindowsDesktop) {
        AppendOutputLine(LocalizedEditorString("Build.RunUnsupportedPlatform", "Run failed: direct launch is only supported for Windows Desktop build profiles."));
        m_BackgroundBuildTask.reset();
        return false;
    }

    if (kind == EBackgroundBuildTaskKind::Run) {
        const std::filesystem::path executablePath = FindBuiltExecutablePath(*project, *selectedProfile);
        if (executablePath.empty()) {
            AppendOutputLine(LocalizedEditorString("Build.RunExecutableNotFound", "Run failed: executable not found. Build the project first."));
            m_BackgroundBuildTask.reset();
            return false;
        }
    }

    const std::string taskDisplayName = BuildBackgroundTaskDisplayName(static_cast<int>(kind));
    task->StatusText = taskDisplayName + " " + EditorText("Build.Queued", "queued...").Resolve();
    task->bStatusDirty = true;
    m_BackgroundBuildTask = task;

    AppendOutputLine("========================================");
    AppendOutputLine(
        taskDisplayName +
        " " + EditorText("Build.StartedInBackground", "started in background").Resolve() + " [" + task->ProfileName + "].");
    NotifyProjectStateChanged();

    task->Worker = std::thread([task, project]() {
        BuildController controller;
        const auto outputCallback = [task](const std::string& line) {
            if (line.empty()) {
                return;
            }

            std::lock_guard<std::mutex> lock(task->Mutex);
            task->PendingOutputLines.push_back(line);
            HandleBackgroundBuildOutputLine(task, line);
        };

        FBuildResult result;
        switch (task->Kind) {
        case EBackgroundBuildTaskKind::Configure:
            result = controller.ConfigureProject(*project, task->ProfileName, outputCallback);
            break;
        case EBackgroundBuildTaskKind::Build:
            result = controller.BuildProject(*project, task->ProfileName, outputCallback);
            break;
        case EBackgroundBuildTaskKind::Run: {
            const FEditorBuildProfile* profile = project->FindBuildProfile(task->ProfileName);
            std::filesystem::path executablePath;
            if (profile != nullptr) {
                executablePath = FindBuiltExecutablePath(*project, *profile);
            }
            if (profile == nullptr || executablePath.empty()) {
                result.ErrorMessage = EditorText("Build.RunExecutableNotFound", "Run failed: executable not found. Build the project first.").Resolve();
                break;
            }

            result.BuildDirectory = ResolveBuildDirectoryPath(project->GetProjectRoot(), *profile);
            outputCallback("[run:" + profile->Name + "] " + executablePath.string());
            const FProcessExecutionResult processResult = ImWidgetV4::ExecuteProcess(
                executablePath.parent_path(),
                {executablePath.string()},
                outputCallback,
                task->ProcessCancelToken);
            result.bSuccess = processResult.bSuccess;
            result.bCancelled = processResult.bCancelled;
            result.ExitCode = processResult.ExitCode;
            result.ErrorMessage = processResult.ErrorMessage;
            break;
        }
        case EBackgroundBuildTaskKind::Clean:
            result = controller.CleanProject(*project, task->ProfileName, outputCallback);
            break;
        case EBackgroundBuildTaskKind::ClearCache: {
            const FEditorBuildProfile* profile = project->FindBuildProfile(task->ProfileName);
            if (profile == nullptr) {
                result.ErrorMessage = EditorText("Build.ClearCacheFailedProfileNotFound", "Clear cache failed: build profile not found.").Resolve();
                break;
            }
            result = ClearBuildDirectoryCache(*project, *profile, outputCallback);
            break;
        }
        case EBackgroundBuildTaskKind::Rebuild:
            result = controller.RebuildProject(*project, task->ProfileName, outputCallback);
            break;
        default:
            result = controller.BuildProject(*project, task->ProfileName, outputCallback);
            break;
        }

        std::lock_guard<std::mutex> lock(task->Mutex);
        task->Result = std::move(result);
        task->bFinished = true;
        task->bRefreshProjectTreeOnCompletion = task->Result.bSuccess;
        const std::string completedTaskDisplayName =
            BuildBackgroundTaskDisplayName(static_cast<int>(task->Kind));
        UpdateBackgroundBuildTaskStatus(
            task,
            task->Result.bCancelled
                ? (completedTaskDisplayName + " " + EditorText("Build.Stopped", "stopped.").Resolve())
                : task->Result.bSuccess
                ? (completedTaskDisplayName + " " + EditorText("Build.Finished", "finished.").Resolve())
                : (completedTaskDisplayName + " " + EditorText("Build.Failed", "failed.").Resolve()));
    });

    return true;
}

void EditorWorkspaceController::TickBackgroundBuildTask()
{
    const std::shared_ptr<FBackgroundBuildTaskState> task = m_BackgroundBuildTask;
    if (!task) {
        return;
    }

    std::vector<std::string> pendingLines;
    std::string statusLine;
    FBuildResult result;
    bool bFinished = false;
    bool bRefreshProjectTree = false;
    bool bSawFetchContentCacheRemovalFailure = false;

    {
        std::lock_guard<std::mutex> lock(task->Mutex);
        pendingLines.swap(task->PendingOutputLines);
        if (task->bStatusDirty && !task->StatusText.empty()) {
            statusLine = task->StatusText;
            task->bStatusDirty = false;
        }
        bFinished = task->bFinished;
        bRefreshProjectTree = task->bRefreshProjectTreeOnCompletion;
        bSawFetchContentCacheRemovalFailure = task->bSawFetchContentCacheRemovalFailure;
        if (bFinished) {
            result = task->Result;
        }
    }

    if (!statusLine.empty()) {
        AppendOutputLine("[" + EditorText("Build.Status", "status").Resolve() + "] " + statusLine);
        NotifyProjectStateChanged();
    }

    for (const std::string& line : pendingLines) {
        AppendOutputLine(line);
    }

    if (!bFinished) {
        return;
    }

    if (task->Worker.joinable()) {
        task->Worker.join();
    }

    if (result.bCancelled) {
        const std::string completedTaskDisplayName =
            BuildBackgroundTaskDisplayName(static_cast<int>(task->Kind));
        AppendOutputLine(completedTaskDisplayName + " " + EditorText("Build.Stopped", "stopped.").Resolve());
    } else if (result.bSuccess) {
        const std::string completedTaskDisplayName =
            BuildBackgroundTaskDisplayName(static_cast<int>(task->Kind));
        AppendOutputLine(
            completedTaskDisplayName +
            " " + EditorText("Build.Complete", "complete").Resolve() + ": " + result.BuildDirectory.string());
    } else {
        const std::string completedTaskDisplayName =
            BuildBackgroundTaskDisplayName(static_cast<int>(task->Kind));
        AppendOutputLine(
            completedTaskDisplayName + " " + EditorText("Build.FailedWithoutPeriod", "failed").Resolve() + ": " +
            (result.ErrorMessage.empty()
                ? (EditorText("Build.ProcessExitedWithCode", "Process exited with code").Resolve() + " " + std::to_string(result.ExitCode) + ".")
                : result.ErrorMessage));
        if (bSawFetchContentCacheRemovalFailure) {
            AppendOutputLine(EditorText(
                "Build.FetchContentCacheRemovalHint",
                "Hint: CMake could not remove a FetchContent dependency cache directory. Stop any running CMake/MSBuild/Git process, then use Clear Cache for this profile and configure again.").Resolve());
        }
    }

    if (bRefreshProjectTree) {
        RefreshProjectTree();
    }

    m_BackgroundBuildTask.reset();
    NotifyProjectStateChanged();
}

void EditorWorkspaceController::ShutdownBackgroundBuildTask()
{
    const std::shared_ptr<FBackgroundBuildTaskState> task = m_BackgroundBuildTask;
    if (!task) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(task->Mutex);
        if (task->Kind == EBackgroundBuildTaskKind::Run && !task->bFinished && task->ProcessCancelToken) {
            task->ProcessCancelToken->RequestCancel();
        }
    }

    if (task->Worker.joinable()) {
        task->Worker.join();
    }

    m_BackgroundBuildTask.reset();
}

bool EditorWorkspaceController::HasDirtyDocuments() const
{
    for (const FDocumentEntry& entry : m_Documents) {
        if (entry.Session &&
            entry.Session->GetDocument() &&
            entry.Session->GetDocument()->IsDirty()) {
            return true;
        }
    }

    return false;
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

bool EditorWorkspaceController::LoadProjectManifestAtRoot(
    const std::filesystem::path& projectRoot,
    bool bLogErrors)
{
    InvalidateBuildProfileProbeCache();
    m_Project.reset();
    if (projectRoot.empty()) {
        return false;
    }

    const std::filesystem::path manifestPath = EditorProject::BuildManifestFilePath(projectRoot);
    if (!std::filesystem::exists(manifestPath)) {
        return false;
    }

    auto project = std::make_shared<EditorProject>();
    std::string errorMessage;
    if (!project->Load(manifestPath, &errorMessage)) {
        if (bLogErrors) {
            SetLocalizedOutputLine("Project.ManifestLoadFailed", "Project manifest load failed", ": " + errorMessage);
        }
        return false;
    }

    m_Project = project;
    RequestBuildProfileProbeRefresh();
    return true;
}

void EditorWorkspaceController::ClearOpenDocuments()
{
    if (m_DocumentTabs) {
        m_DocumentTabs->ClearTabs();
    }

    m_Documents.clear();
    m_ActiveDocumentIndex = -1;
    if (m_ShellHost) {
        m_ShellHost->SetSession(nullptr);
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

    ImTextOutlineItem* openRootItem = m_ProjectView->AddRootItem(EditorText("Project.OpenDocuments", "Open Documents"));
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
        ImTextOutlineItem* emptyItem = m_ProjectView->AddChildItem(openRootItem, EditorText("Project.NoOpenDocuments", "No open documents"));
        if (emptyItem) {
            m_ProjectView->SetSelectedItem(emptyItem);
        }
    }

    ImTextOutlineItem* recentRootItem = m_ProjectView->AddRootItem(EditorText("Project.RecentFiles", "Recent Files"));
    if (!recentRootItem) {
        return;
    }

    recentRootItem->Expanded = true;
    if (m_RecentFiles.empty()) {
        m_ProjectView->AddChildItem(recentRootItem, EditorText("Project.NoRecentFiles", "No recent files"));
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

    const std::string workspaceRootLabel = m_Project
        ? "Project: " + m_Project->GetProjectName()
        : EditorText("Project.Workspace", "Workspace").Resolve();
    ImTextOutlineItem* workspaceRootItem = m_ProjectView->AddRootItem(workspaceRootLabel);
    if (!workspaceRootItem) {
        return;
    }

    workspaceRootItem->Expanded = true;
    if (m_ProjectRoot.empty() || !std::filesystem::exists(m_ProjectRoot)) {
        m_ProjectView->AddChildItem(workspaceRootItem, EditorText("Project.WorkspaceRootNotConfigured", "Workspace root not configured"));
        return;
    }

    if (m_Project) {
        m_ProjectView->AddChildItem(
            workspaceRootItem,
            "Manifest: " + EditorProject::GetManifestFileName());
        if (!m_Project->GetNamespaceName().empty()) {
            m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Namespace: " + m_Project->GetNamespaceName());
        }
        if (!m_Project->GetTemplateName().empty()) {
            m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Template: " + m_Project->GetTemplateName());
        }
        if (!m_Project->GetStartupDocumentRelativePath().empty()) {
            ImTextOutlineItem* startupItem = m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Startup UI: " + m_Project->GetStartupDocumentRelativePath().generic_string());
            if (startupItem) {
                m_ProjectItemBindings[startupItem] = FProjectItemBinding {
                    EProjectItemKind::WorkspaceFile,
                    -1,
                    m_Project->GetStartupDocumentPath()};
            }
        }
        if (m_Project->GetApplicationSettings().bUseTitleBar &&
            !m_Project->GetTitleBarDocumentRelativePath().empty()) {
            ImTextOutlineItem* titleBarItem = m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Title Bar UI: " + m_Project->GetTitleBarDocumentRelativePath().generic_string());
            if (titleBarItem) {
                m_ProjectItemBindings[titleBarItem] = FProjectItemBinding {
                    EProjectItemKind::WorkspaceFile,
                    -1,
                    m_Project->GetTitleBarDocumentPath()};
            }
        }
        if (!m_Project->GetActiveBuildProfileName().empty()) {
            m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Active Build Profile: " + m_Project->GetActiveBuildProfileName());
        }
        if (!m_Project->GetBuildProfiles().empty()) {
            ImTextOutlineItem* profilesRootItem =
                m_ProjectView->AddChildItem(workspaceRootItem, EditorText("Project.BuildProfiles", "Build Profiles"));
            if (profilesRootItem) {
                profilesRootItem->Expanded = true;
                for (const FEditorBuildProfile& profile : m_Project->GetBuildProfiles()) {
                    FEnvironmentProbeReport probeReport;
                    const bool bHasProbeReport = TryGetBuildProfileProbeReport(profile.Name, probeReport);
                    const bool bRefreshingProbe = IsBuildProfileProbeRefreshing(profile.Name);
                    const std::string readinessLabel = bHasProbeReport
                        ? (probeReport.bReady ? "Ready" : "Needs Setup")
                        : (bRefreshingProbe ? "Refreshing" : "Unknown");

                    std::string label = profile.Name + " [" +
                        GetTargetPlatformDisplayName(profile.TargetPlatform) + " / " +
                        profile.Configuration + " / " + readinessLabel + "]";
                    if (profile.Name == m_Project->GetActiveBuildProfileName()) {
                        label += " *";
                    }

                    ImTextOutlineItem* profileItem = m_ProjectView->AddChildItem(profilesRootItem, label);
                    if (!profileItem) {
                        continue;
                    }

                    m_ProjectItemBindings[profileItem] =
                        FProjectItemBinding {EProjectItemKind::BuildProfile, -1, {}, profile.Name};

                    if (bHasProbeReport) {
                        for (const FEnvironmentProbeItem& probeItem : probeReport.Items) {
                            std::string probeLabel =
                                probeItem.Label + " [" + ToDisplayString(probeItem.Status) + "]";
                            if (!probeItem.Details.empty()) {
                                probeLabel += " - " + probeItem.Details;
                            }
                            m_ProjectView->AddChildItem(profileItem, probeLabel);
                        }
                    } else {
                        m_ProjectView->AddChildItem(
                            profileItem,
                            bRefreshingProbe
                                ? EditorText("Project.RefreshingEnvironmentProbe", "Refreshing environment probe...")
                                : EditorText("Project.ProbeDataUnavailable", "Probe data unavailable"));
                    }
                }
            }
        }
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
        m_ProjectView->AddChildItem(workspaceRootItem, EditorText("Project.NoSupportedFiles", "No supported files"));
    }
}

void EditorWorkspaceController::RequestProjectViewRefresh()
{
    if (m_bBatchUiUpdateActive) {
        m_bProjectViewRefreshPending = true;
        return;
    }

    RebuildProjectView();
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

    if (binding.Kind == EProjectItemKind::BuildProfile) {
        if (!binding.ProfileName.empty()) {
            SetActiveBuildProfile(binding.ProfileName);
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
