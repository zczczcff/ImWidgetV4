#include "build/BuildController.h"
#include "editor/EditorDocument.h"
#include "project/EditorProject.h"
#include "templates/ProjectScaffolder.h"
#include "toolchains/EnvironmentProbe.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

struct FCliOptions {
    std::filesystem::path ProjectRoot = std::filesystem::current_path();
    std::string ProfileName;
    bool bVerbose = false;
};

std::string TrimWhitespaceCopy(const std::string& text)
{
    const std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }

    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
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

bool ContainsPathSeparators(const std::string& text)
{
    return text.find('/') != std::string::npos || text.find('\\') != std::string::npos;
}

std::string NormalizeIdentifier(const std::string& rawText, const std::string& fallback)
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

std::string NormalizeStartupDocumentFileName(const std::string& rawText)
{
    std::string trimmedName = TrimWhitespaceCopy(rawText);
    if (trimmedName.empty()) {
        trimmedName = "Main";
    }
    if (EndsWithCaseInsensitive(trimmedName, ".ui.json")) {
        return trimmedName;
    }

    const std::filesystem::path path(trimmedName);
    const std::string stem = path.stem().string();
    const std::string baseName = stem.empty() ? trimmedName : stem;
    return baseName + ".ui.json";
}

std::string BuildStartupWidgetClassName(const std::string& startupDocumentFileName)
{
    std::string baseName = startupDocumentFileName;
    if (EndsWithCaseInsensitive(baseName, ".ui.json")) {
        baseName.resize(baseName.size() - std::string(".ui.json").size());
    } else {
        baseName = std::filesystem::path(baseName).stem().string();
    }

    std::string className = NormalizeIdentifier(baseName, "MainView");
    if (!className.empty()) {
        className.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(className.front())));
    }
    if (className.size() < 4 || className.substr(className.size() - 4) != "View") {
        className += "View";
    }
    return className;
}

std::shared_ptr<ImWidget> BuildDefaultStartupRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("RootCanvas");
    canvas->SetDesiredSize(FVector2(1280.0f, 720.0f));

    auto title = std::make_shared<ImTextBlock>();
    title->SetName("TitleText");
    title->SetText("ImWidgetV4 App");
    title->SetFontSize(32.0f);
    title->SetWrapText(false);
    if (auto* slot = canvas->AddChildAt(title, FVector2(0.08f, 0.08f))) {
        slot->SetAutoSize(true);
    }

    auto button = std::make_shared<ImButton>();
    button->SetName("PrimaryButton");
    button->SetText("Action");
    if (auto* slot = canvas->AddChildAt(button, FVector2(0.08f, 0.20f))) {
        slot->SetAutoSize(true);
    }

    return canvas;
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

void PrintUsage()
{
    std::cout
        << "Usage: imwidgetv4 <command> [options]\n\n"
        << "Commands:\n"
        << "  project create <parent-dir> <name> [--namespace <name>] [--startup <name>] [--source|--sdk <path>]\n"
        << "  project validate [--project <dir>]\n"
        << "  project profiles [--project <dir>]\n"
        << "  build configure|build|clean|rebuild [--project <dir>] [--profile <name>]\n"
        << "  probe [--project <dir>] [--profile <name>]\n";
}

bool ConsumeOptionValue(
    const std::vector<std::string>& args,
    std::size_t& index,
    const std::string& option,
    std::string& outValue)
{
    if (index + 1 >= args.size()) {
        std::cerr << option << " requires a value.\n";
        return false;
    }
    outValue = args[++index];
    return true;
}

bool ParseCommonOptions(
    const std::vector<std::string>& args,
    std::size_t firstIndex,
    FCliOptions& options)
{
    for (std::size_t index = firstIndex; index < args.size(); ++index) {
        const std::string& arg = args[index];
        std::string value;
        if (arg == "--project" || arg == "-p") {
            if (!ConsumeOptionValue(args, index, arg, value)) {
                return false;
            }
            options.ProjectRoot = std::filesystem::path(value).lexically_normal();
        } else if (arg == "--profile") {
            if (!ConsumeOptionValue(args, index, arg, value)) {
                return false;
            }
            options.ProfileName = value;
        } else if (arg == "--verbose" || arg == "-v") {
            options.bVerbose = true;
        } else {
            std::cerr << "Unknown option: " << arg << "\n";
            return false;
        }
    }
    return true;
}

bool LoadProject(const std::filesystem::path& projectRoot, EditorProject& project)
{
    const std::filesystem::path manifestPath = EditorProject::BuildManifestFilePath(projectRoot);
    std::string error;
    if (!project.Load(manifestPath, &error)) {
        std::cerr << "Failed to load project: " << error << "\n";
        std::cerr << "Manifest: " << manifestPath.string() << "\n";
        return false;
    }
    return true;
}

std::string ResolveProfileName(const EditorProject& project, const std::string& requestedProfile)
{
    return requestedProfile.empty() ? project.GetActiveBuildProfileName() : requestedProfile;
}

int PrintBuildResult(const FBuildResult& result)
{
    if (result.bSuccess) {
        std::cout << "OK";
        if (!result.BuildDirectory.empty()) {
            std::cout << ": " << result.BuildDirectory.string();
        }
        std::cout << "\n";
        return 0;
    }

    if (!result.ErrorMessage.empty()) {
        std::cerr << result.ErrorMessage << "\n";
    }
    return result.ExitCode == 0 ? 1 : (result.ExitCode < 0 ? 1 : result.ExitCode);
}

int RunProjectCreate(const std::vector<std::string>& args)
{
    if (args.size() < 4) {
        std::cerr << "project create requires <parent-dir> and <name>.\n";
        return 1;
    }

    const std::filesystem::path parentDirectory = std::filesystem::path(args[2]).lexically_normal();
    const std::string projectName = TrimWhitespaceCopy(args[3]);
    const std::filesystem::path folderNamePath = std::filesystem::path(projectName).filename();
    if (projectName.empty() || folderNamePath != projectName) {
        std::cerr << "Project name must not be empty and must not contain path separators.\n";
        return 1;
    }

    std::string namespaceName = NormalizeIdentifier(projectName, "AppProject");
    std::string startupDocumentName = "Main";
    FEditorApplicationSettings settings;
    settings.Title = projectName;
    settings.bUseTitleBar = true;
    settings.bShowSystemButtons = true;

    for (std::size_t index = 4; index < args.size(); ++index) {
        std::string value;
        if (args[index] == "--namespace") {
            if (!ConsumeOptionValue(args, index, args[index], value)) {
                return 1;
            }
            namespaceName = NormalizeIdentifier(value, "AppProject");
        } else if (args[index] == "--startup") {
            if (!ConsumeOptionValue(args, index, args[index], value)) {
                return 1;
            }
            startupDocumentName = value;
        } else if (args[index] == "--source") {
            settings.LibraryIntegrationMode = EEditorLibraryIntegrationMode::Source;
            settings.SdkPackagePath.clear();
        } else if (args[index] == "--sdk") {
            if (!ConsumeOptionValue(args, index, args[index], value)) {
                return 1;
            }
            settings.LibraryIntegrationMode = EEditorLibraryIntegrationMode::SDK;
            settings.SdkPackagePath = std::filesystem::path(value).lexically_normal();
        } else {
            std::cerr << "Unknown option: " << args[index] << "\n";
            return 1;
        }
    }

    const std::filesystem::path projectRoot = (parentDirectory / projectName).lexically_normal();
    if (std::filesystem::exists(projectRoot)) {
        std::cerr << "Target folder already exists: " << projectRoot.string() << "\n";
        return 1;
    }

    const std::string startupDocumentFileName = NormalizeStartupDocumentFileName(startupDocumentName);
    if (startupDocumentFileName.empty() ||
        ContainsPathSeparators(startupDocumentFileName) ||
        std::filesystem::path(startupDocumentFileName).filename().string() != startupDocumentFileName) {
        std::cerr << "Startup UI name must not contain path separators.\n";
        return 1;
    }

    const std::filesystem::path startupDocumentRelativePath = std::filesystem::path("ui") / startupDocumentFileName;
    const std::filesystem::path titleBarDocumentRelativePath = std::filesystem::path("ui") / "TitleBar.ui.json";

    std::error_code errorCode;
    std::filesystem::create_directories(projectRoot / "src", errorCode);
    std::filesystem::create_directories(projectRoot / "include", errorCode);
    std::filesystem::create_directories(projectRoot / "ui", errorCode);
    std::filesystem::create_directories(projectRoot / "generated", errorCode);
    std::filesystem::create_directories(projectRoot / "cmake", errorCode);
    if (errorCode) {
        std::cerr << "Failed to create project directories: " << errorCode.message() << "\n";
        return 1;
    }

    EditorDocument startupDocument;
    startupDocument.NewDocument(BuildDefaultStartupRoot(), "Main");
    std::string documentError;
    if (!startupDocument.SaveAs(projectRoot / startupDocumentRelativePath, &documentError)) {
        std::cerr << documentError << "\n";
        return 1;
    }

    EditorDocument titleBarDocument;
    titleBarDocument.NewDocument(BuildDefaultTitleBarRoot(projectName), "TitleBar");
    if (!titleBarDocument.SaveAs(projectRoot / titleBarDocumentRelativePath, &documentError)) {
        std::cerr << documentError << "\n";
        return 1;
    }

    settings.TitleBarDocumentRelativePath = titleBarDocumentRelativePath;

    FProjectScaffoldRequest scaffoldRequest;
    scaffoldRequest.ProjectRoot = projectRoot;
    scaffoldRequest.ProjectName = projectName;
    scaffoldRequest.NamespaceName = namespaceName;
    scaffoldRequest.TemplateName = "Blank App";
    scaffoldRequest.StartupDocumentFileName = startupDocumentFileName;
    scaffoldRequest.StartupWidgetClassName = BuildStartupWidgetClassName(startupDocumentFileName);
    scaffoldRequest.TitleBarWidgetClassName = "TitleBarView";
    scaffoldRequest.ApplicationSettings = settings;
    scaffoldRequest.StartupRootWidget = startupDocument.GetRootWidget();
    scaffoldRequest.TitleBarRootWidget = titleBarDocument.GetRootWidget();

    const FProjectScaffoldResult scaffoldResult = ProjectScaffolder::Scaffold(scaffoldRequest);
    if (!scaffoldResult.bSuccess) {
        std::cerr << scaffoldResult.ErrorMessage << "\n";
        return 1;
    }

    EditorProject project;
    if (!project.CreateNew(projectRoot, projectName, namespaceName, startupDocumentRelativePath, "Blank App")) {
        std::cerr << "Generated invalid project metadata.\n";
        return 1;
    }
    project.SetApplicationSettings(settings);
    std::string saveError;
    if (!project.Save(&saveError)) {
        std::cerr << saveError << "\n";
        return 1;
    }

    std::cout << "Created project: " << projectRoot.string() << "\n";
    return 0;
}

int RunProjectCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "project requires a subcommand.\n";
        return 1;
    }

    if (args[1] == "create") {
        return RunProjectCreate(args);
    }

    FCliOptions options;
    if (!ParseCommonOptions(args, 2, options)) {
        return 1;
    }

    EditorProject project;
    if (!LoadProject(options.ProjectRoot, project)) {
        return 1;
    }

    if (args[1] == "validate") {
        std::cout << "Project is valid: " << project.GetProjectName() << "\n";
        std::cout << "Root: " << project.GetProjectRoot().string() << "\n";
        std::cout << "Active profile: " << project.GetActiveBuildProfileName() << "\n";
        return 0;
    }

    if (args[1] == "profiles") {
        for (const FEditorBuildProfile& profile : project.GetBuildProfiles()) {
            std::cout
                << (profile.Name == project.GetActiveBuildProfileName() ? "* " : "  ")
                << profile.Name
                << " [" << GetTargetPlatformDisplayName(profile.TargetPlatform)
                << ", " << profile.Configuration;
            if (profile.TargetPlatform == EEditorTargetPlatform::WindowsDesktop) {
                std::cout << ", " << NormalizeWindowsArchitecture(profile.WindowsSettings.Architecture);
            }
            std::cout << "]\n";
        }
        return 0;
    }

    std::cerr << "Unknown project subcommand: " << args[1] << "\n";
    return 1;
}

int RunBuildCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "build requires a subcommand.\n";
        return 1;
    }

    FCliOptions options;
    if (!ParseCommonOptions(args, 2, options)) {
        return 1;
    }

    EditorProject project;
    if (!LoadProject(options.ProjectRoot, project)) {
        return 1;
    }

    const std::string profileName = ResolveProfileName(project, options.ProfileName);
    BuildController controller;
    const BuildController::FOutputCallback outputCallback = [](const std::string& line) {
        std::cout << line << "\n";
    };

    if (args[1] == "configure") {
        return PrintBuildResult(controller.ConfigureProject(project, profileName, outputCallback));
    }
    if (args[1] == "build") {
        return PrintBuildResult(controller.BuildProject(project, profileName, outputCallback));
    }
    if (args[1] == "clean") {
        return PrintBuildResult(controller.CleanProject(project, profileName, outputCallback));
    }
    if (args[1] == "rebuild") {
        return PrintBuildResult(controller.RebuildProject(project, profileName, outputCallback));
    }

    std::cerr << "Unknown build subcommand: " << args[1] << "\n";
    return 1;
}

int RunProbeCommand(const std::vector<std::string>& args)
{
    FCliOptions options;
    if (!ParseCommonOptions(args, 1, options)) {
        return 1;
    }

    EditorProject project;
    if (!LoadProject(options.ProjectRoot, project)) {
        return 1;
    }

    const FEditorBuildProfile* profile = project.FindBuildProfile(ResolveProfileName(project, options.ProfileName));
    if (profile == nullptr) {
        std::cerr << "Build profile was not found.\n";
        return 1;
    }

    const FEnvironmentProbeReport report = EnvironmentProbe::Probe(*profile);
    std::cout << "Target: " << GetTargetPlatformDisplayName(report.TargetPlatform) << "\n";
    for (const FEnvironmentProbeItem& item : report.Items) {
        std::cout << item.Label << ": " << ToDisplayString(item.Status) << " - " << item.Details << "\n";
    }
    return report.bReady ? 0 : 1;
}

} // namespace

int main(int argc, char** argv)
{
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    for (int index = 1; index < argc; ++index) {
        args.emplace_back(argv[index]);
    }

    if (args.empty() || args[0] == "help" || args[0] == "--help" || args[0] == "-h") {
        PrintUsage();
        return args.empty() ? 1 : 0;
    }

    if (args[0] == "project") {
        return RunProjectCommand(args);
    }
    if (args[0] == "build") {
        return RunBuildCommand(args);
    }
    if (args[0] == "probe") {
        return RunProbeCommand(args);
    }

    std::cerr << "Unknown command: " << args[0] << "\n";
    PrintUsage();
    return 1;
}
