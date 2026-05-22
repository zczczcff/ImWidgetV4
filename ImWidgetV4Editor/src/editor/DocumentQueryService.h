#pragma once

#include "EditorDocument.h"

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

struct FUiTreeNodeInfo {
    std::string WidgetId;
    std::string ParentWidgetId;
    std::string TypeName;
    std::string Name;
    std::string RoleName;
    std::string CodegenMemberAccess;
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

struct FUiLintDiagnostic {
    std::string Severity;
    std::string Code;
    std::string Message;
    std::string WidgetId;
    std::string TypeName;
    std::string FieldName;
};

struct FUiLintInfo {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::vector<FUiLintDiagnostic> Diagnostics;
};

FUiTreeNodeInfo BuildTreeNodeInfo(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    EditorDocument& document,
    std::size_t depth);
void AppendTreeNode(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    EditorDocument& document,
    std::size_t depth,
    FUiDocumentTreeInfo& outInfo);
std::size_t CalculateWidgetDepth(
    EditorDocument& document,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
FUiLintDiagnostic MakeLintDiagnostic(
    const std::string& severity,
    const std::string& code,
    const std::string& message,
    const std::string& widgetId = std::string(),
    const std::string& typeName = std::string(),
    const std::string& fieldName = std::string());

FUiDocumentTreeInfo BuildDocumentTreeInfo(const std::filesystem::path& inputPath);
FUiDocumentTreeInfo FindNodes(const std::filesystem::path& inputPath, const FUiFindRequest& request);
FUiNodeInspectInfo InspectNode(const std::filesystem::path& inputPath, const std::string& widgetId);
FUiLintInfo LintDocumentFile(const std::filesystem::path& inputPath);

} // namespace ImWidgetV4Editor
