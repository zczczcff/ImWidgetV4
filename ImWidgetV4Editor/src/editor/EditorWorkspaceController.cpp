#include "EditorWorkspaceController.h"

#include "EditorSession.h"
#include "EditorPaths.h"
#include "EditorShellHost.h"
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
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
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

std::vector<std::string> GetAvailableProjectTemplateNames()
{
    return {"Blank App"};
}

std::string BuildBackgroundTaskDisplayName(int kind)
{
    switch (kind) {
    case 0:
        return "Configure";
    case 1:
        return "Build";
    case 2:
        return "Clean";
    case 3:
        return "Rebuild";
    default:
        return "Build";
    }
}

bool StartsWith(const std::string& text, const std::string& prefix)
{
    return text.size() >= prefix.size() &&
           std::equal(prefix.begin(), prefix.end(), text.begin());
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

    if (StartsWith(line, "[configure]")) {
        UpdateBackgroundBuildTaskStatus(task, "Configuring project...");
        return;
    }

    if (StartsWith(line, "[build]")) {
        UpdateBackgroundBuildTaskStatus(task, "Starting build...");
        return;
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
    options.Title = "Select Project Root";
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Unsupported && m_OutputText) {
            m_OutputText->SetItems({"Select project root is unsupported by the active platform backend."});
        } else if (dialogResult.Code == EPathDialogResultCode::Error && m_OutputText) {
            m_OutputText->SetItems({"Select project root failed: " + dialogResult.ErrorMessage});
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
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project failed: project name must not contain path separators."});
        }
        return false;
    }

    const std::string trimmedNamespaceName = TrimWhitespaceCopy(options.NamespaceName);
    if (trimmedNamespaceName.empty()) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project failed: namespace must not be empty."});
        }
        return false;
    }

    const std::string normalizedStartupDocumentFileName =
        NormalizeStartupDocumentFileName(options.StartupDocumentName);
    if (normalizedStartupDocumentFileName.empty() ||
        ContainsPathSeparators(normalizedStartupDocumentFileName) ||
        std::filesystem::path(normalizedStartupDocumentFileName).filename().string() != normalizedStartupDocumentFileName) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project failed: startup UI name must not contain path separators."});
        }
        return false;
    }

    const std::string trimmedTemplateName = TrimWhitespaceCopy(options.TemplateName).empty()
        ? std::string("Blank App")
        : TrimWhitespaceCopy(options.TemplateName);

    if (HasDirtyDocuments()) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project blocked: save or discard dirty documents first."});
        }
        return false;
    }

    try {
        const std::filesystem::path projectRoot = (parentDirectory / folderNamePath).lexically_normal();
        if (std::filesystem::exists(projectRoot)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create project failed: target folder already exists."});
            }
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
        const std::string startupWidgetClassName =
            BuildStartupWidgetClassName(normalizedStartupDocumentFileName);
        std::shared_ptr<EditorSession> bootstrapSession = CreateSession();
        if (!bootstrapSession || !bootstrapSession->GetDocument()) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create project failed: could not create startup document session."});
            }
            return false;
        }

        bootstrapSession->GetDocument()->SetDisplayTitle(
            GetDocumentDisplayTitleFromFileName(normalizedStartupDocumentFileName));
        std::string documentError;
        if (!bootstrapSession->GetDocument()->SaveAs(startupDocumentPath, &documentError)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create project failed: " + documentError});
            }
            return false;
        }

        FProjectScaffoldRequest scaffoldRequest;
        scaffoldRequest.ProjectRoot = projectRoot;
        scaffoldRequest.ProjectName = trimmedProjectName;
        scaffoldRequest.NamespaceName = trimmedNamespaceName;
        scaffoldRequest.TemplateName = trimmedTemplateName;
        scaffoldRequest.StartupDocumentFileName = normalizedStartupDocumentFileName;
        scaffoldRequest.StartupWidgetClassName = startupWidgetClassName;
        scaffoldRequest.StartupRootWidget = bootstrapSession->GetDocument()->GetRootWidget();
        const FProjectScaffoldResult scaffoldResult = ProjectScaffolder::Scaffold(scaffoldRequest);
        if (!scaffoldResult.bSuccess) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create project failed: " + scaffoldResult.ErrorMessage});
            }
            return false;
        }

        auto project = std::make_shared<EditorProject>();
        if (!project->CreateNew(
                projectRoot,
                trimmedProjectName,
                trimmedNamespaceName,
                startupDocumentRelativePath,
                trimmedTemplateName)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create project failed: invalid project metadata."});
            }
            return false;
        }

        std::string manifestError;
        if (!project->Save(&manifestError)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Create project failed: " + manifestError});
            }
            return false;
        }

        ClearOpenDocuments();
        m_Project = project;
        m_ProjectRoot = projectRoot;
        RememberRecentFile(startupDocumentPath);

        std::shared_ptr<EditorSession> session = CreateSession();
        if (!session || !session->OpenDocumentFromPath(startupDocumentPath) || !AddSession(session, true)) {
            if (m_OutputText) {
                m_OutputText->SetItems({"Project created, but opening the startup document failed."});
            }
            RebuildProjectView();
            NotifyProjectStateChanged();
            return false;
        }

        RebuildProjectView();
        NotifyProjectStateChanged();
        if (m_OutputText) {
            m_OutputText->SetItems({"Created app project " + trimmedProjectName + " [" + trimmedTemplateName + "]"});
        }
        return true;
    } catch (const std::exception& error) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project failed: " + std::string(error.what())});
        }
        return false;
    } catch (...) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project failed."});
        }
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

bool EditorWorkspaceController::NewAppProject(ImApplication& app)
{
    if (HasDirtyDocuments()) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Create project blocked: save or discard dirty documents first."});
        }
        return false;
    }

    FOpenFolderDialogOptions options;
    options.Title = "Select New Project Parent Directory";
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Unsupported && m_OutputText) {
            m_OutputText->SetItems({"Create project is unsupported by the active platform backend."});
        } else if (dialogResult.Code == EPathDialogResultCode::Error && m_OutputText) {
            m_OutputText->SetItems({"Create project failed: " + dialogResult.ErrorMessage});
        }
        return false;
    }

    OpenCreateAppProjectDialog(app, dialogResult.Path);
    return true;
}

bool EditorWorkspaceController::OpenAppProject(ImApplication& app)
{
    if (HasDirtyDocuments()) {
        if (m_OutputText) {
            m_OutputText->SetItems({"Open project blocked: save or discard dirty documents first."});
        }
        return false;
    }

    FOpenFolderDialogOptions options;
    options.Title = "Select App Project Root";
    options.InitialDirectory = m_ProjectRoot.empty() ? GetDefaultEditorWorkspaceDirectory() : m_ProjectRoot;

    const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
    if (!dialogResult.IsAccepted()) {
        if (dialogResult.Code == EPathDialogResultCode::Unsupported && m_OutputText) {
            m_OutputText->SetItems({"Open project is unsupported by the active platform backend."});
        } else if (dialogResult.Code == EPathDialogResultCode::Error && m_OutputText) {
            m_OutputText->SetItems({"Open project failed: " + dialogResult.ErrorMessage});
        }
        return false;
    }

    return OpenAppProjectAt(dialogResult.Path);
}

bool EditorWorkspaceController::OpenProjectSettings(ImApplication& app)
{
    if (!m_Project) {
        AppendOutputLine("Project settings unavailable: no active project.");
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
        if (m_OutputText) {
            m_OutputText->SetItems({"Open project blocked: save or discard dirty documents first."});
        }
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
            m_OutputText->SetItems({"Open project failed: " + manifestError});
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
                m_OutputText->SetItems({"Project opened, but startup document could not be loaded."});
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
        m_OutputText->SetItems({"Opened app project " + project->GetProjectName()});
    }
    return true;
}

bool EditorWorkspaceController::ConfigureProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Configure failed: project root not configured.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Configure, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::ConfigureProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Configure failed: project root not configured.");
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine("Configure failed: build profile not found.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Configure, resolvedProfileName);
}

bool EditorWorkspaceController::BuildProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Build failed: project root not configured.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Build, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::BuildProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Build failed: project root not configured.");
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine("Build failed: build profile not found.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Build, resolvedProfileName);
}

bool EditorWorkspaceController::CleanProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Clean failed: project root not configured.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Clean, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::CleanProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Clean failed: project root not configured.");
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine("Clean failed: build profile not found.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Clean, resolvedProfileName);
}

bool EditorWorkspaceController::RebuildProject()
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Rebuild failed: project root not configured.");
        return false;
    }

    return StartBackgroundBuildTask(EBackgroundBuildTaskKind::Rebuild, m_Project->GetActiveBuildProfileName());
}

bool EditorWorkspaceController::RebuildProject(const std::string& profileName)
{
    if (!m_Project || m_ProjectRoot.empty()) {
        AppendOutputLine("Rebuild failed: project root not configured.");
        return false;
    }

    const std::string resolvedProfileName =
        profileName.empty() ? m_Project->GetActiveBuildProfileName() : profileName;
    if (resolvedProfileName.empty() || m_Project->FindBuildProfile(resolvedProfileName) == nullptr) {
        AppendOutputLine("Rebuild failed: build profile not found.");
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
        AppendOutputLine("Failed to save active build profile: " + saveError);
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
        AppendOutputLine("Failed to save build profile changes: " + saveError);
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

std::vector<std::string> EditorWorkspaceController::GetOutputLines() const
{
    return m_OutputText ? m_OutputText->GetItems() : std::vector<std::string>();
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
    dialogOptions.HeadingText = "Create App Project";
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
    dialogOptions.ConfirmText = "Create";
    dialogOptions.CancelText = "Cancel";
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
    dialogOptions.BuildProfiles = m_Project->GetBuildProfiles();
    dialogOptions.ActiveBuildProfileName = m_Project->GetActiveBuildProfileName();
    dialogOptions.OnConfirm = [weakThis = weak_from_this()](const FEditorBuildProfile& profile, bool bMakeActive) {
        if (auto self = weakThis.lock()) {
            return self->UpdateBuildProfile(profile, bMakeActive);
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

    if (binding.Kind == EProjectItemKind::BuildProfile) {
        const bool bBuildRunning = IsBuildTaskRunning();
        const bool bIsActiveProfile = m_Project && binding.ProfileName == m_Project->GetActiveBuildProfileName();

        items.push_back(FPopupMenuItem {
            bIsActiveProfile ? "Active Profile" : "Set Active Profile",
            {},
            {},
            !bBuildRunning && !binding.ProfileName.empty(),
            false,
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->SetActiveBuildProfile(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }
        });
        items.push_back(FPopupMenuItem {
            "Configure This Profile",
            {},
            {},
            !bBuildRunning && !binding.ProfileName.empty(),
            false,
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->ConfigureProject(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }
        });
        items.push_back(FPopupMenuItem {
            "Build This Profile",
            {},
            {},
            !bBuildRunning && !binding.ProfileName.empty(),
            false,
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->BuildProject(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }
        });
        items.push_back(FPopupMenuItem {
            "Reveal Build Folder",
            {},
            {},
            !binding.ProfileName.empty(),
            false,
            [weakThis, profileName = binding.ProfileName]() {
                if (auto self = weakThis.lock()) {
                    self->RevealProjectBuildDirectory(profileName);
                    self->CloseProjectItemContextMenu();
                }
            }
        });
        items.push_back(FPopupMenuItem {"", {}, {}, true, true, {}});
        items.push_back(FPopupMenuItem {
            "Project Settings...",
            {},
            {},
            true,
            false,
            [weakThis, application = &app]() {
                if (auto self = weakThis.lock()) {
                    if (application) {
                        self->OpenProjectSettings(*application);
                    }
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
        LoadProjectManifestAtRoot(m_ProjectRoot, false);
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
        AppendOutputLine("Build request ignored: another background build task is already running.");
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
    const std::string taskDisplayName = BuildBackgroundTaskDisplayName(static_cast<int>(kind));
    task->StatusText = taskDisplayName + " queued...";
    task->bStatusDirty = true;
    m_BackgroundBuildTask = task;

    AppendOutputLine("========================================");
    AppendOutputLine(
        taskDisplayName +
        " started in background [" + task->ProfileName + "].");
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
        case EBackgroundBuildTaskKind::Clean:
            result = controller.CleanProject(*project, task->ProfileName, outputCallback);
            break;
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
            task->Result.bSuccess
                ? (completedTaskDisplayName + " finished.")
                : (completedTaskDisplayName + " failed."));
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

    {
        std::lock_guard<std::mutex> lock(task->Mutex);
        pendingLines.swap(task->PendingOutputLines);
        if (task->bStatusDirty && !task->StatusText.empty()) {
            statusLine = task->StatusText;
            task->bStatusDirty = false;
        }
        bFinished = task->bFinished;
        bRefreshProjectTree = task->bRefreshProjectTreeOnCompletion;
        if (bFinished) {
            result = task->Result;
        }
    }

    if (!statusLine.empty()) {
        AppendOutputLine("[status] " + statusLine);
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

    if (result.bSuccess) {
        const std::string completedTaskDisplayName =
            BuildBackgroundTaskDisplayName(static_cast<int>(task->Kind));
        AppendOutputLine(
            completedTaskDisplayName +
            " complete: " + result.BuildDirectory.string());
    } else {
        const std::string completedTaskDisplayName =
            BuildBackgroundTaskDisplayName(static_cast<int>(task->Kind));
        AppendOutputLine(
            completedTaskDisplayName + " failed: " +
            (result.ErrorMessage.empty()
                ? ("Process exited with code " + std::to_string(result.ExitCode) + ".")
                : result.ErrorMessage));
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
        if (bLogErrors && m_OutputText) {
            m_OutputText->SetItems({"Project manifest load failed: " + errorMessage});
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

    const std::string workspaceRootLabel = m_Project
        ? "Project: " + m_Project->GetProjectName()
        : std::string("Workspace");
    ImTextOutlineItem* workspaceRootItem = m_ProjectView->AddRootItem(workspaceRootLabel);
    if (!workspaceRootItem) {
        return;
    }

    workspaceRootItem->Expanded = true;
    if (m_ProjectRoot.empty() || !std::filesystem::exists(m_ProjectRoot)) {
        m_ProjectView->AddChildItem(workspaceRootItem, "Workspace root not configured");
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
            m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Startup UI: " + m_Project->GetStartupDocumentRelativePath().generic_string());
        }
        if (!m_Project->GetActiveBuildProfileName().empty()) {
            m_ProjectView->AddChildItem(
                workspaceRootItem,
                "Active Build Profile: " + m_Project->GetActiveBuildProfileName());
        }
        if (!m_Project->GetBuildProfiles().empty()) {
            ImTextOutlineItem* profilesRootItem =
                m_ProjectView->AddChildItem(workspaceRootItem, "Build Profiles");
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
                            bRefreshingProbe ? "Refreshing environment probe..." : "Probe data unavailable");
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
        m_ProjectView->AddChildItem(workspaceRootItem, "No supported files");
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
