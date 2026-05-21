#include "build/BuildController.h"
#include "cli/UiDocumentCli.h"
#include "editor/DocumentSnapshotExporter.h"
#include "editor/EditorDocument.h"
#include "project/EditorProject.h"
#include "serialization/WidgetCatalog.h"
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
#include <optional>
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
        << "Usage: ImWidgetEditorCLI <command> [options]\n\n"
        << "Commands:\n"
        << "  ui controls list [--json]\n"
        << "  ui controls describe <type-name> [--json]\n"
        << "  ui schema dump [--json]\n"
        << "  ui validate <input.ui.json> [--json]\n"
        << "  ui format <input.ui.json> [--json]\n"
        << "  ui tree <input.ui.json> [--json]\n"
        << "  ui find <input.ui.json> [--id <id>] [--type <type>] [--name <name>] [--json]\n"
        << "  ui get <input.ui.json> <widget-id> [property] [--json]\n"
        << "  ui inspect <input.ui.json> <widget-id> [--json]\n"
        << "  ui rename <input.ui.json> <widget-id> <name> [--json]\n"
        << "  ui set <input.ui.json> <widget-id> <property> <value> [--json]\n"
        << "  ui add <input.ui.json> <parent-widget-id> <widget-type> [--json]\n"
        << "  ui remove <input.ui.json> <widget-id> [--json]\n"
        << "  ui duplicate <input.ui.json> <widget-id> [--json]\n"
        << "  ui move <input.ui.json> <widget-id> <new-parent-widget-id> [--json]\n"
        << "  project create <parent-dir> <name> [--namespace <name>] [--startup <name>] [--source|--sdk <path>]\n"
        << "  project validate [--project <dir>]\n"
        << "  project profiles [--project <dir>]\n"
        << "  project settings get|set [--project <dir>] [key value]\n"
        << "  codegen regenerate|reinit-main [--project <dir>]\n"
        << "  snapshot export <input.ui.json> <output.png> [--width <n>] [--height <n>]\n"
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

bool BuildScaffoldRequestFromProject(EditorProject& project, FProjectScaffoldRequest& outRequest)
{
    if (project.GetProjectRoot().empty()) {
        std::cerr << "Project root is empty.\n";
        return false;
    }
    if (project.GetStartupDocumentRelativePath().empty()) {
        std::cerr << "Project startup document is not configured.\n";
        return false;
    }

    EditorDocument startupDocument;
    std::string loadError;
    if (!startupDocument.Load(project.GetStartupDocumentPath(), &loadError)) {
        std::cerr << "Failed to load startup document: " << loadError << "\n";
        return false;
    }
    if (!startupDocument.GetRootWidget()) {
        std::cerr << "Startup document has no root widget.\n";
        return false;
    }

    FProjectScaffoldRequest request;
    request.ProjectRoot = project.GetProjectRoot();
    request.ProjectName = project.GetProjectName();
    request.NamespaceName = project.GetNamespaceName();
    request.TemplateName = project.GetTemplateName();
    request.StartupDocumentFileName = project.GetStartupDocumentRelativePath().filename().string();
    request.StartupWidgetClassName = BuildStartupWidgetClassName(request.StartupDocumentFileName);
    request.TitleBarWidgetClassName = "TitleBarView";
    request.ApplicationSettings = project.GetApplicationSettings();
    if (request.ApplicationSettings.Title.empty()) {
        request.ApplicationSettings.Title = request.ProjectName;
    }
    request.StartupRootWidget = startupDocument.GetRootWidget();

    if (request.ApplicationSettings.bUseTitleBar) {
        if (request.ApplicationSettings.TitleBarDocumentRelativePath.empty()) {
            request.ApplicationSettings.TitleBarDocumentRelativePath =
                std::filesystem::path("ui") / "TitleBar.ui.json";
        }

        const std::filesystem::path titleBarDocumentPath =
            (project.GetProjectRoot() / request.ApplicationSettings.TitleBarDocumentRelativePath).lexically_normal();
        EditorDocument titleBarDocument;
        if (!std::filesystem::exists(titleBarDocumentPath)) {
            titleBarDocument.NewDocument(BuildDefaultTitleBarRoot(request.ProjectName), "TitleBar");
            std::string saveError;
            if (!titleBarDocument.SaveAs(titleBarDocumentPath, &saveError)) {
                std::cerr << "Failed to create title bar document: " << saveError << "\n";
                return false;
            }

            FEditorApplicationSettings updatedSettings = project.GetApplicationSettings();
            updatedSettings.TitleBarDocumentRelativePath = request.ApplicationSettings.TitleBarDocumentRelativePath;
            project.SetApplicationSettings(updatedSettings);
            std::string projectSaveError;
            if (!project.Save(&projectSaveError)) {
                std::cerr << "Failed to save project manifest: " << projectSaveError << "\n";
                return false;
            }
        } else if (!titleBarDocument.Load(titleBarDocumentPath, &loadError)) {
            std::cerr << "Failed to load title bar document: " << loadError << "\n";
            return false;
        }

        auto titleBarRoot = std::dynamic_pointer_cast<ImTitleBar>(titleBarDocument.GetRootWidget());
        if (!titleBarRoot) {
            std::cerr << "Title bar document root must be ImTitleBar.\n";
            return false;
        }
        titleBarRoot->SetShowSystemButtons(request.ApplicationSettings.bShowSystemButtons);
        request.TitleBarRootWidget = titleBarRoot;
    }

    outRequest = std::move(request);
    return true;
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

std::string BoolToString(bool value)
{
    return value ? "true" : "false";
}

bool ParseBool(const std::string& text, bool& outValue)
{
    std::string normalized = text;
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        outValue = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        outValue = false;
        return true;
    }
    return false;
}

bool ParsePositiveInteger(const std::string& text, int& outValue)
{
    try {
        std::size_t processed = 0;
        const int value = std::stoi(text, &processed, 10);
        if (processed != text.size() || value <= 0) {
            return false;
        }

        outValue = value;
        return true;
    } catch (...) {
        return false;
    }
}

std::string PropertyKindToString(ImWidgetV4::Reflection::EPropertyKind kind)
{
    switch (kind) {
    case ImWidgetV4::Reflection::EPropertyKind::Int: return "Int";
    case ImWidgetV4::Reflection::EPropertyKind::Float: return "Float";
    case ImWidgetV4::Reflection::EPropertyKind::Bool: return "Bool";
    case ImWidgetV4::Reflection::EPropertyKind::String: return "String";
    case ImWidgetV4::Reflection::EPropertyKind::Color: return "Color";
    case ImWidgetV4::Reflection::EPropertyKind::Vec2: return "Vec2";
    case ImWidgetV4::Reflection::EPropertyKind::Struct: return "Struct";
    case ImWidgetV4::Reflection::EPropertyKind::StringArray: return "StringArray";
    case ImWidgetV4::Reflection::EPropertyKind::Enum: return "Enum";
    }
    return "Struct";
}

void PrintJsonEscapedString(const std::string& value)
{
    std::cout << '"';
    for (char c : value) {
        switch (c) {
        case '\\': std::cout << "\\\\"; break;
        case '"': std::cout << "\\\""; break;
        case '\b': std::cout << "\\b"; break;
        case '\f': std::cout << "\\f"; break;
        case '\n': std::cout << "\\n"; break;
        case '\r': std::cout << "\\r"; break;
        case '\t': std::cout << "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20) {
                std::cout << "\\u";
                constexpr char hex[] = "0123456789abcdef";
                const unsigned char valueByte = static_cast<unsigned char>(c);
                std::cout << '0' << '0' << hex[(valueByte >> 4) & 0x0F] << hex[valueByte & 0x0F];
            } else {
                std::cout << c;
            }
            break;
        }
    }
    std::cout << '"';
}

void PrintWidgetControlListJson(const std::vector<std::string>& widgetTypes)
{
    std::cout << "{\n  \"widgets\": [";
    for (std::size_t index = 0; index < widgetTypes.size(); ++index) {
        if (index > 0) {
            std::cout << ", ";
        }
        PrintJsonEscapedString(widgetTypes[index]);
    }
    std::cout << "]\n}\n";
}

void PrintWidgetControlListText(const std::vector<std::string>& widgetTypes)
{
    for (const std::string& typeName : widgetTypes) {
        std::cout << typeName << "\n";
    }
}

void PrintWidgetControlDescriptionJson(const FWidgetTypeInfo& info)
{
    std::cout << "{\n";
    std::cout << "  \"type\": ";
    PrintJsonEscapedString(info.TypeName);
    std::cout << ",\n  \"properties\": [\n";
    for (std::size_t index = 0; index < info.Properties.size(); ++index) {
        const FWidgetPropertyInfo& property = info.Properties[index];
        std::cout << "    {\n";
        std::cout << "      \"ownerType\": ";
        PrintJsonEscapedString(property.OwnerTypeName);
        std::cout << ",\n      \"name\": ";
        PrintJsonEscapedString(property.Name);
        std::cout << ",\n      \"valueType\": ";
        PrintJsonEscapedString(property.ValueTypeName);
        std::cout << ",\n      \"kind\": ";
        PrintJsonEscapedString(PropertyKindToString(property.Kind));
        std::cout << ",\n      \"description\": ";
        PrintJsonEscapedString(property.Description);
        std::cout << ",\n      \"inherited\": " << (property.bIsInherited ? "true" : "false");
        std::cout << ",\n      \"enumOptions\": [";
        for (std::size_t optionIndex = 0; optionIndex < property.EnumOptions.size(); ++optionIndex) {
            if (optionIndex > 0) {
                std::cout << ", ";
            }
            PrintJsonEscapedString(property.EnumOptions[optionIndex]);
        }
        std::cout << "]\n    }";
        if (index + 1 < info.Properties.size()) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
}

void PrintWidgetSchemaDumpJson(const WidgetCatalog& catalog)
{
    const std::vector<std::string> widgetTypes = catalog.ListWidgetTypes();
    std::cout << "{\n  \"widgets\": [\n";
    bool bPrintedAny = false;
    for (const std::string& typeName : widgetTypes) {
        FWidgetTypeInfo info;
        if (!catalog.TryDescribeWidgetType(typeName, info)) {
            continue;
        }

        if (bPrintedAny) {
            std::cout << ",\n";
        }
        bPrintedAny = true;
        std::cout << "    {\n";
        std::cout << "      \"type\": ";
        PrintJsonEscapedString(info.TypeName);
        std::cout << ",\n      \"properties\": [\n";
        for (std::size_t index = 0; index < info.Properties.size(); ++index) {
            const FWidgetPropertyInfo& property = info.Properties[index];
            std::cout << "        {\n";
            std::cout << "          \"ownerType\": ";
            PrintJsonEscapedString(property.OwnerTypeName);
            std::cout << ",\n          \"name\": ";
            PrintJsonEscapedString(property.Name);
            std::cout << ",\n          \"valueType\": ";
            PrintJsonEscapedString(property.ValueTypeName);
            std::cout << ",\n          \"kind\": ";
            PrintJsonEscapedString(PropertyKindToString(property.Kind));
            std::cout << ",\n          \"description\": ";
            PrintJsonEscapedString(property.Description);
            std::cout << ",\n          \"inherited\": " << (property.bIsInherited ? "true" : "false");
            std::cout << ",\n          \"enumOptions\": [";
            for (std::size_t optionIndex = 0; optionIndex < property.EnumOptions.size(); ++optionIndex) {
                if (optionIndex > 0) {
                    std::cout << ", ";
                }
                PrintJsonEscapedString(property.EnumOptions[optionIndex]);
            }
            std::cout << "]\n        }";
            if (index + 1 < info.Properties.size()) {
                std::cout << ",";
            }
            std::cout << "\n";
        }
        std::cout << "      ]\n    }";
    }
    std::cout << "\n  ]\n}\n";
}

void PrintWidgetControlDescriptionText(const FWidgetTypeInfo& info)
{
    std::cout << "Type: " << info.TypeName << "\n";
    std::cout << "Properties:\n";
    for (const FWidgetPropertyInfo& property : info.Properties) {
        std::cout
            << "  - " << property.OwnerTypeName << "::" << property.Name
            << " [" << property.ValueTypeName << ", " << PropertyKindToString(property.Kind) << "]";
        if (property.bIsInherited) {
            std::cout << " inherited";
        }
        if (!property.Description.empty()) {
            std::cout << " - " << property.Description;
        }
        if (!property.EnumOptions.empty()) {
            std::cout << " {";
            for (std::size_t index = 0; index < property.EnumOptions.size(); ++index) {
                if (index > 0) {
                    std::cout << ", ";
                }
                std::cout << property.EnumOptions[index];
            }
            std::cout << "}";
        }
        std::cout << "\n";
    }
}

void PrintUiValidationJson(const std::filesystem::path& inputPath, bool bSuccess, const std::string& errorMessage)
{
    std::cout << "{\n";
    std::cout << "  \"success\": " << (bSuccess ? "true" : "false") << ",\n";
    std::cout << "  \"file\": ";
    PrintJsonEscapedString(inputPath.string());
    if (!bSuccess) {
        std::cout << ",\n  \"error\": ";
        PrintJsonEscapedString(errorMessage);
    }
    std::cout << "\n}\n";
}

void PrintUiTreeJson(const std::filesystem::path& inputPath, const FUiDocumentTreeInfo& info)
{
    std::cout << "{\n";
    std::cout << "  \"success\": " << (info.bSuccess ? "true" : "false") << ",\n";
    std::cout << "  \"file\": ";
    PrintJsonEscapedString(inputPath.string());
    if (!info.bSuccess) {
        std::cout << ",\n  \"error\": ";
        PrintJsonEscapedString(info.ErrorMessage);
        std::cout << "\n}\n";
        return;
    }

    std::cout << ",\n  \"nodes\": [\n";
    for (std::size_t index = 0; index < info.Nodes.size(); ++index) {
        const FUiTreeNodeInfo& node = info.Nodes[index];
        std::cout << "    {\n";
        std::cout << "      \"path\": ";
        PrintJsonEscapedString(node.WidgetId);
        std::cout << ",\n      \"id\": ";
        PrintJsonEscapedString(node.WidgetId);
        std::cout << ",\n      \"type\": ";
        PrintJsonEscapedString(node.TypeName);
        std::cout << ",\n      \"name\": ";
        PrintJsonEscapedString(node.Name);
        std::cout << ",\n      \"role\": ";
        PrintJsonEscapedString(node.RoleName);
        std::cout << ",\n      \"depth\": " << node.Depth << "\n";
        std::cout << "    }";
        if (index + 1 < info.Nodes.size()) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
}

void PrintUiTreeText(const FUiDocumentTreeInfo& info)
{
    for (const FUiTreeNodeInfo& node : info.Nodes) {
        for (std::size_t depth = 0; depth < node.Depth; ++depth) {
            std::cout << "  ";
        }
        std::cout << node.WidgetId << " " << node.TypeName;
        if (!node.Name.empty()) {
            std::cout << " \"" << node.Name << "\"";
        }
        if (!node.RoleName.empty()) {
            std::cout << " [" << node.RoleName << "]";
        }
        std::cout << "\n";
    }
}

void PrintUiNodeJsonFields(const FUiTreeNodeInfo& node, const char* indent)
{
    std::cout << indent << "\"path\": ";
    PrintJsonEscapedString(node.WidgetId);
    std::cout << ",\n" << indent << "\"id\": ";
    PrintJsonEscapedString(node.WidgetId);
    std::cout << ",\n" << indent << "\"type\": ";
    PrintJsonEscapedString(node.TypeName);
    std::cout << ",\n" << indent << "\"name\": ";
    PrintJsonEscapedString(node.Name);
    std::cout << ",\n" << indent << "\"role\": ";
    PrintJsonEscapedString(node.RoleName);
    std::cout << ",\n" << indent << "\"depth\": " << node.Depth;
}

void PrintUiInspectJson(const std::filesystem::path& inputPath, const FUiNodeInspectInfo& info)
{
    std::cout << "{\n";
    std::cout << "  \"success\": " << (info.bSuccess ? "true" : "false") << ",\n";
    std::cout << "  \"file\": ";
    PrintJsonEscapedString(inputPath.string());
    if (!info.bSuccess) {
        std::cout << ",\n  \"error\": ";
        PrintJsonEscapedString(info.ErrorMessage);
        std::cout << "\n}\n";
        return;
    }

    std::cout << ",\n  \"node\": {\n";
    PrintUiNodeJsonFields(info.Node, "    ");
    std::cout << "\n  },\n";
    std::cout << "  \"properties\": " << info.Properties.dump(2) << ",\n";
    std::cout << "  \"children\": [\n";
    for (std::size_t index = 0; index < info.Children.size(); ++index) {
        std::cout << "    {\n";
        PrintUiNodeJsonFields(info.Children[index], "      ");
        std::cout << "\n    }";
        if (index + 1 < info.Children.size()) {
            std::cout << ",";
        }
        std::cout << "\n";
    }
    std::cout << "  ]\n}\n";
}

void PrintUiInspectText(const FUiNodeInspectInfo& info)
{
    std::cout << "Node: " << info.Node.WidgetId << " " << info.Node.TypeName;
    if (!info.Node.Name.empty()) {
        std::cout << " \"" << info.Node.Name << "\"";
    }
    std::cout << "\nProperties:\n";
    if (info.Properties.is_object()) {
        for (auto it = info.Properties.begin(); it != info.Properties.end(); ++it) {
            std::cout << "  " << it.key() << " = " << it.value().dump() << "\n";
        }
    }
    std::cout << "Children:\n";
    for (const FUiTreeNodeInfo& child : info.Children) {
        std::cout << "  " << child.WidgetId << " " << child.TypeName;
        if (!child.Name.empty()) {
            std::cout << " \"" << child.Name << "\"";
        }
        if (!child.RoleName.empty()) {
            std::cout << " [" << child.RoleName << "]";
        }
        std::cout << "\n";
    }
}

void PrintUiMutationJson(const std::filesystem::path& inputPath, const FUiMutationResult& result)
{
    std::cout << "{\n";
    std::cout << "  \"success\": " << (result.bSuccess ? "true" : "false") << ",\n";
    std::cout << "  \"file\": ";
    PrintJsonEscapedString(inputPath.string());
    if (!result.bSuccess) {
        std::cout << ",\n  \"error\": ";
        PrintJsonEscapedString(result.ErrorMessage);
        std::cout << "\n}\n";
        return;
    }

    std::cout << ",\n  \"changed\": " << (result.bChanged ? "true" : "false") << ",\n";
    std::cout << "  \"node\": {\n";
    PrintUiNodeJsonFields(result.Node, "    ");
    std::cout << "\n  }\n}\n";
}

void PrintUiMutationText(const FUiMutationResult& result)
{
    std::cout
        << (result.bChanged ? "Updated: " : "Unchanged: ")
        << result.Node.WidgetId << " " << result.Node.TypeName;
    if (!result.Node.Name.empty()) {
        std::cout << " \"" << result.Node.Name << "\"";
    }
    std::cout << "\n";
}

json ParseCliJsonOrStringValue(const std::string& text)
{
    try {
        return json::parse(text);
    } catch (...) {
        return text;
    }
}

bool PrintProjectSetting(const EditorProject& project, const std::string& key)
{
    const FEditorApplicationSettings& settings = project.GetApplicationSettings();
    bool bPrintedAny = false;
    const auto printValue = [](const std::string& name, const std::string& value) {
        std::cout << name << "=" << value << "\n";
    };

    if (key.empty() || key == "title") {
        printValue("title", settings.Title);
        bPrintedAny = true;
    }
    if (key.empty() || key == "libraryMode") {
        printValue(
            "libraryMode",
            settings.LibraryIntegrationMode == EEditorLibraryIntegrationMode::SDK ? "SDK" : "Source");
        bPrintedAny = true;
    }
    if (key.empty() || key == "sdkPath") {
        printValue("sdkPath", settings.SdkPackagePath.generic_string());
        bPrintedAny = true;
    }
    if (key.empty() || key == "minimumSdkVersion") {
        printValue("minimumSdkVersion", settings.MinimumSdkVersion);
        bPrintedAny = true;
    }
    if (key.empty() || key == "initialWidth") {
        printValue("initialWidth", std::to_string(settings.InitialWidth));
        bPrintedAny = true;
    }
    if (key.empty() || key == "initialHeight") {
        printValue("initialHeight", std::to_string(settings.InitialHeight));
        bPrintedAny = true;
    }
    if (key.empty() || key == "useTitleBar") {
        printValue("useTitleBar", BoolToString(settings.bUseTitleBar));
        bPrintedAny = true;
    }
    if (key.empty() || key == "showSystemButtons") {
        printValue("showSystemButtons", BoolToString(settings.bShowSystemButtons));
        bPrintedAny = true;
    }
    if (key.empty() || key == "titleBarDocument") {
        printValue("titleBarDocument", settings.TitleBarDocumentRelativePath.generic_string());
        bPrintedAny = true;
    }
    if (!bPrintedAny) {
        std::cerr << "Unknown project setting: " << key << "\n";
    }
    return bPrintedAny;
}

bool SetProjectSetting(EditorProject& project, const std::string& key, const std::string& value)
{
    FEditorApplicationSettings settings = project.GetApplicationSettings();

    if (key == "title") {
        settings.Title = value;
    } else if (key == "libraryMode") {
        if (value == "SDK" || value == "sdk") {
            settings.LibraryIntegrationMode = EEditorLibraryIntegrationMode::SDK;
        } else if (value == "Source" || value == "source") {
            settings.LibraryIntegrationMode = EEditorLibraryIntegrationMode::Source;
        } else {
            std::cerr << "libraryMode must be SDK or Source.\n";
            return false;
        }
    } else if (key == "sdkPath") {
        settings.SdkPackagePath = std::filesystem::path(value).lexically_normal();
        settings.LibraryIntegrationMode = value.empty()
            ? EEditorLibraryIntegrationMode::Source
            : EEditorLibraryIntegrationMode::SDK;
    } else if (key == "minimumSdkVersion") {
        settings.MinimumSdkVersion = value;
    } else if (key == "initialWidth") {
        settings.InitialWidth = std::max(1, std::stoi(value));
    } else if (key == "initialHeight") {
        settings.InitialHeight = std::max(1, std::stoi(value));
    } else if (key == "useTitleBar") {
        if (!ParseBool(value, settings.bUseTitleBar)) {
            std::cerr << "useTitleBar must be a boolean.\n";
            return false;
        }
    } else if (key == "showSystemButtons") {
        if (!ParseBool(value, settings.bShowSystemButtons)) {
            std::cerr << "showSystemButtons must be a boolean.\n";
            return false;
        }
    } else if (key == "titleBarDocument") {
        settings.TitleBarDocumentRelativePath = std::filesystem::path(value).lexically_normal();
        if (settings.TitleBarDocumentRelativePath.is_absolute()) {
            std::cerr << "titleBarDocument must be project-relative.\n";
            return false;
        }
    } else {
        std::cerr << "Unknown project setting: " << key << "\n";
        return false;
    }

    project.SetApplicationSettings(settings);
    std::string saveError;
    if (!project.Save(&saveError)) {
        std::cerr << saveError << "\n";
        return false;
    }

    return true;
}

int RunProjectSettingsCommand(const std::vector<std::string>& args)
{
    if (args.size() < 3) {
        std::cerr << "project settings requires get or set.\n";
        return 1;
    }

    FCliOptions options;
    std::vector<std::string> positional;
    for (std::size_t index = 3; index < args.size(); ++index) {
        std::string value;
        if (args[index] == "--project" || args[index] == "-p") {
            if (!ConsumeOptionValue(args, index, args[index], value)) {
                return 1;
            }
            options.ProjectRoot = std::filesystem::path(value).lexically_normal();
        } else {
            positional.push_back(args[index]);
        }
    }

    EditorProject project;
    if (!LoadProject(options.ProjectRoot, project)) {
        return 1;
    }

    if (args[2] == "get") {
        const std::string key = positional.empty() ? std::string() : positional.front();
        return PrintProjectSetting(project, key) ? 0 : 1;
    }

    if (args[2] == "set") {
        if (positional.size() < 2) {
            std::cerr << "project settings set requires <key> <value>.\n";
            return 1;
        }
        try {
            if (!SetProjectSetting(project, positional[0], positional[1])) {
                return 1;
            }
        } catch (const std::exception& error) {
            std::cerr << error.what() << "\n";
            return 1;
        }
        std::cout << "Updated " << positional[0] << "\n";
        return 0;
    }

    std::cerr << "Unknown project settings subcommand: " << args[2] << "\n";
    return 1;
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
    if (args[1] == "settings") {
        return RunProjectSettingsCommand(args);
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

int RunUiCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "ui requires a subcommand.\n";
        return 1;
    }

    const bool bJsonOutput = std::find(args.begin(), args.end(), "--json") != args.end();

    if (args[1] == "validate") {
        if (args.size() < 3) {
            std::cerr << "ui validate requires <input.ui.json>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        std::string error;
        const bool bSuccess = UiDocumentCli::ValidateDocumentFile(inputPath, &error);
        if (bJsonOutput) {
            PrintUiValidationJson(inputPath, bSuccess, error);
        } else if (bSuccess) {
            std::cout << "OK: " << inputPath.string() << "\n";
        } else {
            std::cerr << "Invalid UI document: " << error << "\n";
        }
        return bSuccess ? 0 : 1;
    }

    if (args[1] == "format") {
        if (args.size() < 3) {
            std::cerr << "ui format requires <input.ui.json>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiMutationResult formatResult = UiDocumentCli::FormatDocumentFile(inputPath);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, formatResult);
        } else if (formatResult.bSuccess) {
            PrintUiMutationText(formatResult);
        } else {
            std::cerr << "Failed to format UI document: " << formatResult.ErrorMessage << "\n";
        }
        return formatResult.bSuccess ? 0 : 1;
    }

    if (args[1] == "tree") {
        if (args.size() < 3) {
            std::cerr << "ui tree requires <input.ui.json>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiDocumentTreeInfo treeInfo = UiDocumentCli::BuildDocumentTreeInfo(inputPath);
        if (bJsonOutput) {
            PrintUiTreeJson(inputPath, treeInfo);
        } else if (treeInfo.bSuccess) {
            PrintUiTreeText(treeInfo);
        } else {
            std::cerr << "Failed to read UI tree: " << treeInfo.ErrorMessage << "\n";
        }
        return treeInfo.bSuccess ? 0 : 1;
    }

    if (args[1] == "schema") {
        if (args.size() < 3 || args[2] != "dump") {
            std::cerr << "ui schema requires dump.\n";
            return 1;
        }

        const WidgetCatalog& catalog = WidgetCatalog::Get();
        if (bJsonOutput) {
            PrintWidgetSchemaDumpJson(catalog);
        } else {
            PrintWidgetControlListText(catalog.ListWidgetTypes());
        }
        return 0;
    }

    if (args[1] == "find") {
        if (args.size() < 3) {
            std::cerr << "ui find requires <input.ui.json>.\n";
            return 1;
        }

        FUiFindRequest request;
        for (std::size_t index = 3; index < args.size(); ++index) {
            std::string value;
            if (args[index] == "--json") {
                continue;
            }
            if (args[index] == "--id") {
                if (!ConsumeOptionValue(args, index, args[index], value)) {
                    return 1;
                }
                request.WidgetId = value;
            } else if (args[index] == "--type") {
                if (!ConsumeOptionValue(args, index, args[index], value)) {
                    return 1;
                }
                request.TypeName = value;
            } else if (args[index] == "--name") {
                if (!ConsumeOptionValue(args, index, args[index], value)) {
                    return 1;
                }
                request.Name = value;
            } else {
                std::cerr << "Unknown option: " << args[index] << "\n";
                return 1;
            }
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiDocumentTreeInfo findInfo = UiDocumentCli::FindNodes(inputPath, request);
        if (bJsonOutput) {
            PrintUiTreeJson(inputPath, findInfo);
        } else if (findInfo.bSuccess) {
            PrintUiTreeText(findInfo);
        } else {
            std::cerr << "Failed to find UI nodes: " << findInfo.ErrorMessage << "\n";
        }
        return findInfo.bSuccess ? 0 : 1;
    }

    if (args[1] == "get") {
        if (args.size() < 4) {
            std::cerr << "ui get requires <input.ui.json> and <widget-id>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiNodeInspectInfo inspectInfo = UiDocumentCli::InspectNode(inputPath, args[3]);
        if (!inspectInfo.bSuccess) {
            if (bJsonOutput) {
                PrintUiInspectJson(inputPath, inspectInfo);
            } else {
                std::cerr << "Failed to get UI node: " << inspectInfo.ErrorMessage << "\n";
            }
            return 1;
        }

        const bool bHasPropertyName = args.size() >= 5 && args[4] != "--json";
        if (!bHasPropertyName) {
            if (bJsonOutput) {
                PrintUiInspectJson(inputPath, inspectInfo);
            } else {
                PrintUiInspectText(inspectInfo);
            }
            return 0;
        }

        const std::string propertyName = args[4];
        const std::string qualifiedSuffix = "::" + propertyName;
        auto propertyIt = inspectInfo.Properties.end();
        for (auto it = inspectInfo.Properties.begin(); it != inspectInfo.Properties.end(); ++it) {
            const std::string key = it.key();
            if (key == propertyName ||
                (key.size() >= qualifiedSuffix.size() &&
                 key.compare(key.size() - qualifiedSuffix.size(), qualifiedSuffix.size(), qualifiedSuffix) == 0)) {
                propertyIt = it;
                break;
            }
        }
        if (propertyIt == inspectInfo.Properties.end()) {
            std::cerr << "Property was not found: " << propertyName << "\n";
            return 1;
        }

        if (bJsonOutput) {
            std::cout << "{\n  \"success\": true,\n  \"file\": ";
            PrintJsonEscapedString(inputPath.string());
            std::cout << ",\n  \"id\": ";
            PrintJsonEscapedString(args[3]);
            std::cout << ",\n  \"property\": ";
            PrintJsonEscapedString(propertyIt.key());
            std::cout << ",\n  \"value\": " << propertyIt.value().dump(2) << "\n}\n";
        } else {
            std::cout << propertyIt.value().dump() << "\n";
        }
        return 0;
    }

    if (args[1] == "inspect") {
        if (args.size() < 4) {
            std::cerr << "ui inspect requires <input.ui.json> and <widget-id>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiNodeInspectInfo inspectInfo = UiDocumentCli::InspectNode(inputPath, args[3]);
        if (bJsonOutput) {
            PrintUiInspectJson(inputPath, inspectInfo);
        } else if (inspectInfo.bSuccess) {
            PrintUiInspectText(inspectInfo);
        } else {
            std::cerr << "Failed to inspect UI node: " << inspectInfo.ErrorMessage << "\n";
        }
        return inspectInfo.bSuccess ? 0 : 1;
    }

    if (args[1] == "rename") {
        if (args.size() < 5) {
            std::cerr << "ui rename requires <input.ui.json>, <widget-id>, and <name>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiMutationResult renameResult = UiDocumentCli::RenameNode(inputPath, args[3], args[4]);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, renameResult);
        } else if (renameResult.bSuccess) {
            PrintUiMutationText(renameResult);
        } else {
            std::cerr << "Failed to rename UI node: " << renameResult.ErrorMessage << "\n";
        }
        return renameResult.bSuccess ? 0 : 1;
    }

    if (args[1] == "set") {
        if (args.size() < 6) {
            std::cerr << "ui set requires <input.ui.json>, <widget-id>, <property>, and <value>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const json value = ParseCliJsonOrStringValue(args[5]);
        const FUiMutationResult setResult =
            UiDocumentCli::SetNodeProperty(inputPath, args[3], args[4], value);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, setResult);
        } else if (setResult.bSuccess) {
            PrintUiMutationText(setResult);
        } else {
            std::cerr << "Failed to set UI property: " << setResult.ErrorMessage << "\n";
        }
        return setResult.bSuccess ? 0 : 1;
    }

    if (args[1] == "add") {
        if (args.size() < 5) {
            std::cerr << "ui add requires <input.ui.json>, <parent-widget-id>, and <widget-type>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiMutationResult addResult = UiDocumentCli::AddNode(inputPath, args[3], args[4]);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, addResult);
        } else if (addResult.bSuccess) {
            PrintUiMutationText(addResult);
        } else {
            std::cerr << "Failed to add UI node: " << addResult.ErrorMessage << "\n";
        }
        return addResult.bSuccess ? 0 : 1;
    }

    if (args[1] == "remove") {
        if (args.size() < 4) {
            std::cerr << "ui remove requires <input.ui.json> and <widget-id>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiMutationResult removeResult = UiDocumentCli::RemoveNode(inputPath, args[3]);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, removeResult);
        } else if (removeResult.bSuccess) {
            PrintUiMutationText(removeResult);
        } else {
            std::cerr << "Failed to remove UI node: " << removeResult.ErrorMessage << "\n";
        }
        return removeResult.bSuccess ? 0 : 1;
    }

    if (args[1] == "duplicate") {
        if (args.size() < 4) {
            std::cerr << "ui duplicate requires <input.ui.json> and <widget-id>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiMutationResult duplicateResult = UiDocumentCli::DuplicateNode(inputPath, args[3]);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, duplicateResult);
        } else if (duplicateResult.bSuccess) {
            PrintUiMutationText(duplicateResult);
        } else {
            std::cerr << "Failed to duplicate UI node: " << duplicateResult.ErrorMessage << "\n";
        }
        return duplicateResult.bSuccess ? 0 : 1;
    }

    if (args[1] == "move") {
        if (args.size() < 5) {
            std::cerr << "ui move requires <input.ui.json>, <widget-id>, and <new-parent-widget-id>.\n";
            return 1;
        }

        const std::filesystem::path inputPath = std::filesystem::path(args[2]).lexically_normal();
        const FUiMutationResult moveResult = UiDocumentCli::MoveNode(inputPath, args[3], args[4]);
        if (bJsonOutput) {
            PrintUiMutationJson(inputPath, moveResult);
        } else if (moveResult.bSuccess) {
            PrintUiMutationText(moveResult);
        } else {
            std::cerr << "Failed to move UI node: " << moveResult.ErrorMessage << "\n";
        }
        return moveResult.bSuccess ? 0 : 1;
    }

    if (args[1] != "controls") {
        std::cerr << "Unknown ui subcommand: " << args[1] << "\n";
        return 1;
    }

    if (args.size() < 3) {
        std::cerr << "ui controls requires list or describe.\n";
        return 1;
    }

    const WidgetCatalog& catalog = WidgetCatalog::Get();

    if (args[2] == "list") {
        const std::vector<std::string> widgetTypes = catalog.ListWidgetTypes();
        if (bJsonOutput) {
            PrintWidgetControlListJson(widgetTypes);
        } else {
            PrintWidgetControlListText(widgetTypes);
        }
        return 0;
    }

    if (args[2] == "describe") {
        if (args.size() < 4) {
            std::cerr << "ui controls describe requires <type-name>.\n";
            return 1;
        }

        FWidgetTypeInfo info;
        if (!catalog.TryDescribeWidgetType(args[3], info)) {
            std::cerr << "Unknown or unsupported widget type: " << args[3] << "\n";
            return 1;
        }

        if (bJsonOutput) {
            PrintWidgetControlDescriptionJson(info);
        } else {
            PrintWidgetControlDescriptionText(info);
        }
        return 0;
    }

    std::cerr << "Unknown ui controls subcommand: " << args[2] << "\n";
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

int RunCodegenCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "codegen requires a subcommand.\n";
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

    FProjectScaffoldRequest scaffoldRequest;
    if (!BuildScaffoldRequestFromProject(project, scaffoldRequest)) {
        return 1;
    }

    FProjectScaffoldResult result;
    if (args[1] == "regenerate") {
        result = ProjectScaffolder::GenerateCode(scaffoldRequest);
    } else if (args[1] == "reinit-main") {
        result = ProjectScaffolder::ReinitializeMainCpp(scaffoldRequest);
    } else {
        std::cerr << "Unknown codegen subcommand: " << args[1] << "\n";
        return 1;
    }

    if (!result.bSuccess) {
        std::cerr << result.ErrorMessage << "\n";
        return 1;
    }

    std::cout << "Generated files:\n";
    for (const std::filesystem::path& filePath : result.GeneratedFiles) {
        std::cout << "  " << filePath.string() << "\n";
    }
    return 0;
}

int RunSnapshotCommand(const std::vector<std::string>& args)
{
    if (args.size() < 2) {
        std::cerr << "snapshot requires a subcommand.\n";
        return 1;
    }

    if (args[1] != "export") {
        std::cerr << "Unknown snapshot subcommand: " << args[1] << "\n";
        return 1;
    }

    if (args.size() < 4) {
        std::cerr << "snapshot export requires <input.ui.json> and <output.png>.\n";
        return 1;
    }

    FDocumentSnapshotExportRequest request;
    request.InputPath = std::filesystem::path(args[2]).lexically_normal();
    request.OutputPath = std::filesystem::path(args[3]).lexically_normal();

    std::optional<int> width;
    std::optional<int> height;

    for (std::size_t index = 4; index < args.size(); ++index) {
        if (args[index] == "--width") {
            std::string value;
            if (!ConsumeOptionValue(args, index, args[index], value)) {
                return 1;
            }
            int parsedWidth = 0;
            if (!ParsePositiveInteger(value, parsedWidth)) {
                std::cerr << "--width must be a positive integer.\n";
                return 1;
            }
            width = parsedWidth;
        } else if (args[index] == "--height") {
            std::string value;
            if (!ConsumeOptionValue(args, index, args[index], value)) {
                return 1;
            }
            int parsedHeight = 0;
            if (!ParsePositiveInteger(value, parsedHeight)) {
                std::cerr << "--height must be a positive integer.\n";
                return 1;
            }
            height = parsedHeight;
        } else {
            std::cerr << "Unknown option: " << args[index] << "\n";
            return 1;
        }
    }

    if (width.has_value() != height.has_value()) {
        std::cerr << "--width and --height must be provided together.\n";
        return 1;
    }

    request.Width = width;
    request.Height = height;

    const FDocumentSnapshotExportResult result = DocumentSnapshotExporter::ExportToPng(request);
    if (!result.bSuccess) {
        std::cerr << result.ErrorMessage << "\n";
        return 1;
    }

    std::cout
        << "Exported snapshot: " << result.OutputPath.string()
        << " (" << static_cast<int>(result.ExportSize.X)
        << "x" << static_cast<int>(result.ExportSize.Y) << ")\n";
    return 0;
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
    if (args[0] == "ui") {
        return RunUiCommand(args);
    }
    if (args[0] == "build") {
        return RunBuildCommand(args);
    }
    if (args[0] == "codegen") {
        return RunCodegenCommand(args);
    }
    if (args[0] == "snapshot") {
        return RunSnapshotCommand(args);
    }
    if (args[0] == "probe") {
        return RunProbeCommand(args);
    }

    std::cerr << "Unknown command: " << args[0] << "\n";
    PrintUsage();
    return 1;
}
