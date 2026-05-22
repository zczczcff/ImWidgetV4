#include "AppProjectCreator.h"

#include "ProjectNaming.h"
#include "ProjectScaffoldRequestBuilder.h"
#include "ProjectTemplateDefaults.h"
#include "../editor/EditorDocument.h"
#include "../templates/ProjectScaffolder.h"

#include <filesystem>

namespace ImWidgetV4Editor {

namespace {

FAppProjectCreateResult MakeCreateError(const std::string& message)
{
    FAppProjectCreateResult result;
    result.ErrorMessage = message;
    return result;
}

} // namespace

FAppProjectCreateResult CreateAppProject(const FAppProjectCreateRequest& request)
{
    const std::string trimmedProjectName = TrimWhitespaceCopy(request.ProjectName);
    const std::filesystem::path folderNamePath = std::filesystem::path(trimmedProjectName).filename();
    if (request.ParentDirectory.empty() || trimmedProjectName.empty() || folderNamePath != trimmedProjectName) {
        return MakeCreateError("Project name must not be empty and must not contain path separators.");
    }

    const std::string trimmedNamespaceName = TrimWhitespaceCopy(request.NamespaceName);
    if (trimmedNamespaceName.empty()) {
        return MakeCreateError("Namespace must not be empty.");
    }

    const std::string startupDocumentFileName =
        NormalizeStartupDocumentFileName(request.StartupDocumentName);
    if (startupDocumentFileName.empty() ||
        ContainsPathSeparators(startupDocumentFileName) ||
        std::filesystem::path(startupDocumentFileName).filename().string() != startupDocumentFileName) {
        return MakeCreateError("Startup UI name must not contain path separators.");
    }

    const std::string trimmedTemplateName = TrimWhitespaceCopy(request.TemplateName).empty()
        ? std::string("Blank App")
        : TrimWhitespaceCopy(request.TemplateName);

    try {
        const std::filesystem::path projectRoot = (request.ParentDirectory / folderNamePath).lexically_normal();
        if (std::filesystem::exists(projectRoot)) {
            return MakeCreateError("Target folder already exists: " + projectRoot.string());
        }

        std::filesystem::create_directories(projectRoot / "src");
        std::filesystem::create_directories(projectRoot / "include");
        std::filesystem::create_directories(projectRoot / "ui");
        std::filesystem::create_directories(projectRoot / "generated");
        std::filesystem::create_directories(projectRoot / "cmake");

        const std::filesystem::path startupDocumentRelativePath =
            std::filesystem::path("ui") / startupDocumentFileName;
        const std::filesystem::path startupDocumentPath =
            (projectRoot / startupDocumentRelativePath).lexically_normal();
        const std::filesystem::path titleBarDocumentRelativePath =
            std::filesystem::path("ui") / "TitleBar.ui.json";
        const std::filesystem::path titleBarDocumentPath =
            (projectRoot / titleBarDocumentRelativePath).lexically_normal();

        EditorDocument startupDocument;
        startupDocument.NewDocument(
            BuildDefaultStartupRoot(),
            GetDocumentDisplayTitleFromFileName(startupDocumentFileName));
        std::string documentError;
        if (!startupDocument.SaveAs(startupDocumentPath, &documentError)) {
            return MakeCreateError(documentError);
        }

        EditorDocument titleBarDocument;
        titleBarDocument.NewDocument(BuildDefaultTitleBarRoot(trimmedProjectName), "TitleBar");
        if (!titleBarDocument.SaveAs(titleBarDocumentPath, &documentError)) {
            return MakeCreateError(documentError);
        }

        FEditorApplicationSettings settings = request.ApplicationSettings;
        if (settings.Title.empty()) {
            settings.Title = trimmedProjectName;
        }
        settings.bUseTitleBar = true;
        settings.bShowSystemButtons = true;
        settings.TitleBarDocumentRelativePath = titleBarDocumentRelativePath;

        FProjectScaffoldRequest scaffoldRequest;
        scaffoldRequest.ProjectRoot = projectRoot;
        scaffoldRequest.ProjectName = trimmedProjectName;
        scaffoldRequest.NamespaceName = trimmedNamespaceName;
        scaffoldRequest.TemplateName = trimmedTemplateName;
        scaffoldRequest.StartupDocumentFileName = startupDocumentFileName;
        scaffoldRequest.StartupWidgetClassName = BuildWidgetClassNameFromUiFileName(startupDocumentFileName);
        scaffoldRequest.TitleBarWidgetClassName = "TitleBarView";
        scaffoldRequest.ApplicationSettings = settings;
        scaffoldRequest.StartupRootWidget = startupDocument.GetRootWidget();
        scaffoldRequest.TitleBarRootWidget = titleBarDocument.GetRootWidget();
        scaffoldRequest.StartupRootWidgetJson = LoadRootWidgetJsonFromFile(startupDocumentPath);
        scaffoldRequest.TitleBarRootWidgetJson = LoadRootWidgetJsonFromFile(titleBarDocumentPath);

        const FProjectScaffoldResult scaffoldResult = ProjectScaffolder::Scaffold(scaffoldRequest);
        if (!scaffoldResult.bSuccess) {
            return MakeCreateError(scaffoldResult.ErrorMessage);
        }

        EditorProject project;
        if (!project.CreateNew(
                projectRoot,
                trimmedProjectName,
                trimmedNamespaceName,
                startupDocumentRelativePath,
                trimmedTemplateName)) {
            return MakeCreateError("Generated invalid project metadata.");
        }
        project.SetApplicationSettings(settings);

        std::string manifestError;
        if (!project.Save(&manifestError)) {
            return MakeCreateError(manifestError);
        }

        FAppProjectCreateResult result;
        result.bSuccess = true;
        result.ProjectRoot = projectRoot;
        result.StartupDocumentPath = startupDocumentPath;
        result.StartupDocumentRelativePath = startupDocumentRelativePath;
        result.ProjectName = trimmedProjectName;
        result.TemplateName = trimmedTemplateName;
        return result;
    } catch (const std::exception& error) {
        return MakeCreateError(error.what());
    } catch (...) {
        return MakeCreateError("Create project failed.");
    }
}

} // namespace ImWidgetV4Editor
