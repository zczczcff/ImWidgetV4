#pragma once

#include "../editor/DocumentQueryService.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FUiMutationResult {
    bool bSuccess = false;
    bool bChanged = false;
    std::string ErrorMessage;
    FUiTreeNodeInfo Node;
};

struct FUiPatchOperationResult {
    bool bSuccess = false;
    bool bChanged = false;
    std::string Operation;
    std::string ErrorMessage;
    FUiTreeNodeInfo Node;
};

struct FUiPatchResult {
    bool bSuccess = false;
    bool bChanged = false;
    std::string ErrorMessage;
    std::vector<FUiPatchOperationResult> Operations;
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
    static FUiPatchResult PatchDocumentFile(
        const std::filesystem::path& inputPath,
        const std::filesystem::path& patchPath);
    static FUiDocumentTreeInfo BuildDocumentTreeInfo(const std::filesystem::path& inputPath);
    static FUiDocumentTreeInfo FindNodes(const std::filesystem::path& inputPath, const FUiFindRequest& request);
    static FUiNodeInspectInfo InspectNode(const std::filesystem::path& inputPath, const std::string& widgetId);
    static FUiMutationResult RenameNode(
        const std::filesystem::path& inputPath,
        const std::string& widgetId,
        const std::string& newName);
    static FUiMutationResult SetNodeCodegenMemberAccess(
        const std::filesystem::path& inputPath,
        const std::string& widgetId,
        const std::string& memberAccess);
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
    static FUiLintInfo LintDocumentFile(const std::filesystem::path& inputPath);
};

} // namespace ImWidgetV4Editor
