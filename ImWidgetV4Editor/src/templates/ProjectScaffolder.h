#pragma once

#include "../project/EditorProject.h"
#include "../serialization/DocumentFormat.h"

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
    std::string StartupDocumentFileName = "MainView.ui.json";
    std::string StartupWidgetClassName = "MainView";
    std::string TitleBarWidgetClassName = "TitleBarView";
    FEditorApplicationSettings ApplicationSettings;
    std::shared_ptr<ImWidgetV4::ImWidget> StartupRootWidget;
    std::shared_ptr<ImWidgetV4::ImWidget> TitleBarRootWidget;
    // Prefer the source document JSON for codegen so generated files do not
    // expand object defaults that were not authored in the .ui.json.
    json StartupRootWidgetJson;
    json TitleBarRootWidgetJson;
};

struct FProjectScaffoldResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::vector<std::filesystem::path> GeneratedFiles;
};

class ProjectScaffolder {
public:
    static FProjectScaffoldResult GenerateCMake(const FProjectScaffoldRequest& request);
    static FProjectScaffoldResult GenerateCodePreview(const FProjectScaffoldRequest& request);
    static FProjectScaffoldResult GenerateCode(const FProjectScaffoldRequest& request);
    static FProjectScaffoldResult ReinitializeMainCpp(const FProjectScaffoldRequest& request);
    static FProjectScaffoldResult Scaffold(const FProjectScaffoldRequest& request);
};

} // namespace ImWidgetV4Editor
