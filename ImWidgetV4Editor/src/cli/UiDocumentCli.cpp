#include "UiDocumentCli.h"

#include "../editor/LogicalWidgetTree.h"

#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

FUiTreeNodeInfo BuildTreeNodeInfo(
    const std::shared_ptr<ImWidget>& widget,
    EditorDocument& document,
    std::size_t depth)
{
    FUiTreeNodeInfo node;
    if (!widget) {
        return node;
    }

    node.Depth = depth;
    node.WidgetId = document.GetWidgetId(widget);
    node.TypeName = widget->GetTypeName();
    node.Name = widget->GetName();
    if (const std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget)) {
        const int childIndex = LogicalWidgetTree::FindLogicalChildIndex(parent, widget);
        if (childIndex >= 0) {
            if (const char* roleName = LogicalWidgetTree::GetLogicalChildRoleName(parent, static_cast<std::size_t>(childIndex))) {
                node.RoleName = roleName;
            }
        }
    }

    return node;
}

void AppendTreeNode(
    const std::shared_ptr<ImWidget>& widget,
    EditorDocument& document,
    std::size_t depth,
    FUiDocumentTreeInfo& outInfo)
{
    if (!widget) {
        return;
    }

    outInfo.Nodes.push_back(BuildTreeNodeInfo(widget, document, depth));

    const std::size_t childCount = LogicalWidgetTree::GetLogicalChildCount(widget);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        AppendTreeNode(LogicalWidgetTree::GetLogicalChildAt(widget, childIndex), document, depth + 1, outInfo);
    }
}

} // namespace

bool UiDocumentCli::ValidateDocumentFile(const std::filesystem::path& inputPath, std::string* outError)
{
    EditorDocument document;
    if (!document.Load(inputPath, outError)) {
        return false;
    }
    return true;
}

FUiDocumentTreeInfo UiDocumentCli::BuildDocumentTreeInfo(const std::filesystem::path& inputPath)
{
    FUiDocumentTreeInfo result;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    AppendTreeNode(document.GetRootWidget(), document, 0, result);
    return result;
}

FUiNodeInspectInfo UiDocumentCli::InspectNode(const std::filesystem::path& inputPath, const std::string& widgetId)
{
    FUiNodeInspectInfo result;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    const std::shared_ptr<ImWidget> widget = document.FindWidgetById(widgetId);
    if (!widget) {
        result.ErrorMessage = "Widget id was not found: " + widgetId;
        return result;
    }

    std::size_t depth = 0;
    std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
    while (parent) {
        ++depth;
        parent = document.FindLogicalParent(parent);
    }

    result.bSuccess = true;
    result.Node = BuildTreeNodeInfo(widget, document, depth);
    json widgetJson = widget->ToJson();
    if (widgetJson.is_object() && widgetJson.contains("Properties") && widgetJson["Properties"].is_object()) {
        result.Properties = widgetJson["Properties"];
    }

    const std::size_t childCount = LogicalWidgetTree::GetLogicalChildCount(widget);
    result.Children.reserve(childCount);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        if (std::shared_ptr<ImWidget> child = LogicalWidgetTree::GetLogicalChildAt(widget, childIndex)) {
            result.Children.push_back(BuildTreeNodeInfo(child, document, depth + 1));
        }
    }

    return result;
}

FUiMutationResult UiDocumentCli::RenameNode(
    const std::filesystem::path& inputPath,
    const std::string& widgetId,
    const std::string& newName)
{
    FUiMutationResult result;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    const std::shared_ptr<ImWidget> widget = document.FindWidgetById(widgetId);
    if (!widget) {
        result.ErrorMessage = "Widget id was not found: " + widgetId;
        return result;
    }

    result.bChanged = widget->GetName() != newName;
    widget->SetName(newName);
    document.SetDirty(true);

    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    std::size_t depth = 0;
    std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
    while (parent) {
        ++depth;
        parent = document.FindLogicalParent(parent);
    }

    result.bSuccess = true;
    result.Node = BuildTreeNodeInfo(widget, document, depth);
    return result;
}

} // namespace ImWidgetV4Editor
