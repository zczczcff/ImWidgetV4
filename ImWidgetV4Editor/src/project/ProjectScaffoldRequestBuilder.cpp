#include "ProjectScaffoldRequestBuilder.h"

#include "ProjectNaming.h"
#include "ProjectTemplateDefaults.h"

#include <imwidgetv4/widgets/TitleBar.h>

#include <fstream>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

FProjectScaffoldRequestBuildResult MakeBuildError(const std::string& message)
{
    FProjectScaffoldRequestBuildResult result;
    result.ErrorMessage = message;
    return result;
}

std::shared_ptr<EditorDocument> LoadDocumentForScaffold(
    const std::filesystem::path& documentPath,
    const FProjectScaffoldRequestBuildOptions& options,
    std::string* outError)
{
    if (options.FindOpenDocument) {
        if (auto document = options.FindOpenDocument(documentPath)) {
            return document;
        }
    }

    auto document = std::make_shared<EditorDocument>();
    if (!document->Load(documentPath, outError)) {
        return nullptr;
    }
    return document;
}

bool SaveProjectSettings(
    EditorProject& project,
    const FEditorApplicationSettings& settings,
    const FProjectScaffoldRequestBuildOptions& options,
    std::string* outError)
{
    project.SetApplicationSettings(settings);
    if (!project.Save(outError)) {
        return false;
    }
    if (options.OnApplicationSettingsChanged) {
        options.OnApplicationSettingsChanged(settings);
    }
    return true;
}

} // namespace

json LoadRootWidgetJsonFromFile(const std::filesystem::path& documentPath)
{
    try {
        std::ifstream stream(documentPath);
        if (!stream.is_open()) {
            return json();
        }

        json documentJson;
        stream >> documentJson;
        if (documentJson.is_object() && documentJson.contains("RootWidget")) {
            return documentJson.at("RootWidget");
        }
    } catch (...) {
    }

    return json();
}

FProjectScaffoldRequestBuildResult BuildProjectScaffoldRequest(
    EditorProject& project,
    const FProjectScaffoldRequestBuildOptions& options)
{
    if (project.GetProjectRoot().empty()) {
        return MakeBuildError("Project root is empty.");
    }
    if (project.GetStartupDocumentRelativePath().empty()) {
        return MakeBuildError("Project startup document is not configured.");
    }

    const std::filesystem::path startupDocumentPath = project.GetStartupDocumentPath().lexically_normal();
    std::string documentError;
    std::shared_ptr<EditorDocument> startupDocument =
        LoadDocumentForScaffold(startupDocumentPath, options, &documentError);
    if (!startupDocument) {
        return MakeBuildError("Failed to load startup document: " + documentError);
    }
    if (!startupDocument->GetRootWidget()) {
        return MakeBuildError("Startup document has no root widget.");
    }

    FProjectScaffoldRequest request;
    request.ProjectRoot = project.GetProjectRoot();
    request.ProjectName = project.GetProjectName();
    request.NamespaceName = project.GetNamespaceName();
    request.TemplateName = project.GetTemplateName();
    request.StartupDocumentFileName = project.GetStartupDocumentRelativePath().filename().string();
    request.StartupWidgetClassName = BuildWidgetClassNameFromUiFileName(request.StartupDocumentFileName);
    request.TitleBarWidgetClassName = "TitleBarView";
    request.ApplicationSettings = project.GetApplicationSettings();
    if (request.ApplicationSettings.Title.empty()) {
        request.ApplicationSettings.Title = request.ProjectName;
    }
    request.StartupRootWidget = startupDocument->GetRootWidget();
    if (!startupDocument->IsDirty()) {
        request.StartupRootWidgetJson = LoadRootWidgetJsonFromFile(startupDocumentPath);
    }

    if (request.ApplicationSettings.bUseTitleBar) {
        if (request.ApplicationSettings.TitleBarDocumentRelativePath.empty()) {
            request.ApplicationSettings.TitleBarDocumentRelativePath =
                std::filesystem::path("ui") / "TitleBar.ui.json";
        }

        const std::filesystem::path titleBarDocumentPath =
            (project.GetProjectRoot() / request.ApplicationSettings.TitleBarDocumentRelativePath).lexically_normal();
        std::shared_ptr<EditorDocument> titleBarDocument;
        if (options.FindOpenDocument) {
            titleBarDocument = options.FindOpenDocument(titleBarDocumentPath);
        }

        if (!titleBarDocument) {
            titleBarDocument = std::make_shared<EditorDocument>();
            if (!std::filesystem::exists(titleBarDocumentPath)) {
                titleBarDocument->NewDocument(BuildDefaultTitleBarRoot(request.ProjectName), "TitleBar");
                std::string saveError;
                if (!titleBarDocument->SaveAs(titleBarDocumentPath, &saveError)) {
                    return MakeBuildError("Failed to create title bar document: " + saveError);
                }

                FEditorApplicationSettings updatedSettings = project.GetApplicationSettings();
                updatedSettings.TitleBarDocumentRelativePath =
                    request.ApplicationSettings.TitleBarDocumentRelativePath;
                std::string projectSaveError;
                if (!SaveProjectSettings(project, updatedSettings, options, &projectSaveError)) {
                    return MakeBuildError("Failed to save project manifest: " + projectSaveError);
                }
            } else if (!titleBarDocument->Load(titleBarDocumentPath, &documentError)) {
                return MakeBuildError("Failed to load title bar document: " + documentError);
            }
        }

        auto titleBarRoot = std::dynamic_pointer_cast<ImTitleBar>(titleBarDocument->GetRootWidget());
        if (!titleBarRoot) {
            return MakeBuildError("Title bar document root must be ImTitleBar.");
        }

        titleBarRoot->SetShowSystemButtons(request.ApplicationSettings.bShowSystemButtons);
        if (titleBarDocument->HasFilePath()) {
            std::string saveError;
            if (!titleBarDocument->Save(&saveError)) {
                return MakeBuildError("Failed to save title bar document: " + saveError);
            }
        }

        request.TitleBarRootWidget = titleBarDocument->GetRootWidget();
        if (!titleBarDocument->IsDirty()) {
            request.TitleBarRootWidgetJson = LoadRootWidgetJsonFromFile(titleBarDocumentPath);
            if (request.TitleBarRootWidgetJson.contains("Properties") &&
                request.TitleBarRootWidgetJson["Properties"].is_object()) {
                request.TitleBarRootWidgetJson["Properties"]["ImTitleBar::ShowSystemButtons"] =
                    request.ApplicationSettings.bShowSystemButtons;
            }
        }
    }

    FProjectScaffoldRequestBuildResult result;
    result.bSuccess = true;
    result.Request = std::move(request);
    return result;
}

} // namespace ImWidgetV4Editor
