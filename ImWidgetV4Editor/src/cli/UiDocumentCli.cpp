#include "UiDocumentCli.h"

#include "../editor/LogicalWidgetTree.h"

#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

void AppendTreeNode(
    const std::shared_ptr<ImWidget>& widget,
    EditorDocument& document,
    std::size_t depth,
    FUiDocumentTreeInfo& outInfo)
{
    if (!widget) {
        return;
    }

    FUiTreeNodeInfo node;
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

    outInfo.Nodes.push_back(std::move(node));

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

} // namespace ImWidgetV4Editor
