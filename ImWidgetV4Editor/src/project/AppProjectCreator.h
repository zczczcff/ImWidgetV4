#pragma once

#include "EditorProject.h"

#include <filesystem>
#include <string>

namespace ImWidgetV4Editor {

struct FAppProjectCreateRequest {
    std::filesystem::path ParentDirectory;
    std::string ProjectName;
    std::string NamespaceName;
    std::string StartupDocumentName = "MainView";
    std::string TemplateName = "Blank App";
    FEditorApplicationSettings ApplicationSettings;
};

struct FAppProjectCreateResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::filesystem::path ProjectRoot;
    std::filesystem::path StartupDocumentPath;
    std::filesystem::path StartupDocumentRelativePath;
    std::string ProjectName;
    std::string TemplateName;
};

FAppProjectCreateResult CreateAppProject(const FAppProjectCreateRequest& request);

} // namespace ImWidgetV4Editor
