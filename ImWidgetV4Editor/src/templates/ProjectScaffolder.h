#pragma once

#include "../project/EditorProject.h"

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

struct FProjectScaffoldRequest {
    std::filesystem::path ProjectRoot;
    std::string ProjectName;
    std::string NamespaceName;
    std::string TemplateName = "Blank App";
    std::string StartupDocumentFileName = "Main.ui.json";
    std::string StartupWidgetClassName = "MainView";
    FEditorApplicationSettings ApplicationSettings;
    std::shared_ptr<ImWidgetV4::ImWidget> StartupRootWidget;
};

struct FProjectScaffoldResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::vector<std::filesystem::path> GeneratedFiles;
};

class ProjectScaffolder {
public:
    static FProjectScaffoldResult GenerateCode(const FProjectScaffoldRequest& request);
    static FProjectScaffoldResult Scaffold(const FProjectScaffoldRequest& request);
};

} // namespace ImWidgetV4Editor
