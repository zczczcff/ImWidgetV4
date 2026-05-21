#pragma once

#include "../editor/EditorDocument.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FUiTreeNodeInfo {
    std::string WidgetId;
    std::string TypeName;
    std::string Name;
    std::string RoleName;
    std::size_t Depth = 0;
};

struct FUiDocumentTreeInfo {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::vector<FUiTreeNodeInfo> Nodes;
};

class UiDocumentCli {
public:
    static bool ValidateDocumentFile(const std::filesystem::path& inputPath, std::string* outError = nullptr);
    static FUiDocumentTreeInfo BuildDocumentTreeInfo(const std::filesystem::path& inputPath);
};

} // namespace ImWidgetV4Editor
