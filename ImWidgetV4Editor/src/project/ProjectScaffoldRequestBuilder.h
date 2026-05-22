#pragma once

#include "../editor/EditorDocument.h"
#include "../templates/ProjectScaffolder.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string>

namespace ImWidgetV4Editor {

struct FProjectScaffoldRequestBuildOptions {
    std::function<std::shared_ptr<EditorDocument>(const std::filesystem::path& documentPath)> FindOpenDocument;
    std::function<void(const FEditorApplicationSettings& settings)> OnApplicationSettingsChanged;
};

struct FProjectScaffoldRequestBuildResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    FProjectScaffoldRequest Request;
};

json LoadRootWidgetJsonFromFile(const std::filesystem::path& documentPath);
FProjectScaffoldRequestBuildResult BuildProjectScaffoldRequest(
    EditorProject& project,
    const FProjectScaffoldRequestBuildOptions& options = {});

} // namespace ImWidgetV4Editor
