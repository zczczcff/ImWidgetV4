#pragma once

#include "../editor/EditorDocument.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FUiTreeNodeInfo {
    std::string WidgetId;
    std::string ParentWidgetId;
    std::string TypeName;
    std::string Name;
    std::string RoleName;
    std::size_t Depth = 0;
    int ChildIndex = -1;
};

struct FUiDocumentTreeInfo {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::vector<FUiTreeNodeInfo> Nodes;
};

struct FUiFindRequest {
    std::string WidgetId;
    std::string TypeName;
    std::string Name;
};

struct FUiNodeInspectInfo {
    bool bSuccess = false;
    std::string ErrorMessage;
    FUiTreeNodeInfo Node;
    json Properties = json::object();
    std::vector<FUiTreeNodeInfo> Children;
};

struct FUiMutationResult {
    bool bSuccess = false;
    bool bChanged = false;
    std::string ErrorMessage;
    FUiTreeNodeInfo Node;
};

struct FUiNodeDiffEntry {
    std::string Kind;
    std::string WidgetId;
    std::string FieldName;
    FUiTreeNodeInfo BeforeNode;
    FUiTreeNodeInfo AfterNode;
    json BeforeValue = json();
    json AfterValue = json();
};

struct FUiDocumentDiffInfo {
    bool bSuccess = false;
    bool bChanged = false;
    std::string ErrorMessage;
    std::vector<FUiNodeDiffEntry> Entries;
};

class UiDocumentCli {
public:
    static bool ValidateDocumentFile(const std::filesystem::path& inputPath, std::string* outError = nullptr);
    static FUiMutationResult FormatDocumentFile(const std::filesystem::path& inputPath);
    static FUiDocumentTreeInfo BuildDocumentTreeInfo(const std::filesystem::path& inputPath);
    static FUiDocumentTreeInfo FindNodes(const std::filesystem::path& inputPath, const FUiFindRequest& request);
    static FUiNodeInspectInfo InspectNode(const std::filesystem::path& inputPath, const std::string& widgetId);
    static FUiMutationResult RenameNode(
        const std::filesystem::path& inputPath,
        const std::string& widgetId,
        const std::string& newName);
    static FUiMutationResult SetNodeProperty(
        const std::filesystem::path& inputPath,
        const std::string& widgetId,
        const std::string& propertyName,
        const json& value);
    static FUiMutationResult AddNode(
        const std::filesystem::path& inputPath,
        const std::string& parentWidgetId,
        const std::string& widgetTypeName);
    static FUiMutationResult RemoveNode(
        const std::filesystem::path& inputPath,
        const std::string& widgetId);
    static FUiMutationResult DuplicateNode(
        const std::filesystem::path& inputPath,
        const std::string& widgetId);
    static FUiMutationResult MoveNode(
        const std::filesystem::path& inputPath,
        const std::string& widgetId,
        const std::string& newParentWidgetId);
    static FUiDocumentDiffInfo DiffDocuments(
        const std::filesystem::path& beforePath,
        const std::filesystem::path& afterPath);
};

} // namespace ImWidgetV4Editor
