#include "DocumentQueryService.h"

#include "LogicalWidgetTree.h"
#include "../serialization/WidgetCatalog.h"

#include <imwidgetv4/core/Widget.h>

#include <fstream>
#include <map>
#include <set>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

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
    const json codegenMetadata = document.GetWidgetCodegenMetadata(widget);
    node.CodegenMemberAccess = codegenMetadata.value(
        kEditorCodegenMemberAccessFieldName,
        std::string(kEditorCodegenMemberAccessPublic));
    if (const std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget)) {
        node.ParentWidgetId = document.GetWidgetId(parent);
        const int childIndex = LogicalWidgetTree::FindLogicalChildIndex(parent, widget);
        node.ChildIndex = childIndex;
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

std::size_t CalculateWidgetDepth(EditorDocument& document, const std::shared_ptr<ImWidget>& widget)
{
    std::size_t depth = 0;
    std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
    while (parent) {
        ++depth;
        parent = document.FindLogicalParent(parent);
    }
    return depth;
}

FUiLintDiagnostic MakeLintDiagnostic(
    const std::string& severity,
    const std::string& code,
    const std::string& message,
    const std::string& widgetId,
    const std::string& typeName,
    const std::string& fieldName)
{
    FUiLintDiagnostic diagnostic;
    diagnostic.Severity = severity;
    diagnostic.Code = code;
    diagnostic.Message = message;
    diagnostic.WidgetId = widgetId;
    diagnostic.TypeName = typeName;
    diagnostic.FieldName = fieldName;
    return diagnostic;
}

namespace {

bool IsKnownPropertyKey(const FWidgetTypeInfo& typeInfo, const std::string& propertyKey)
{
    std::string ownerTypeName;
    std::string propertyName = propertyKey;
    const std::size_t separator = propertyKey.find("::");
    if (separator != std::string::npos) {
        ownerTypeName = propertyKey.substr(0, separator);
        propertyName = propertyKey.substr(separator + 2);
    }

    for (const FWidgetPropertyInfo& property : typeInfo.Properties) {
        if (property.Name != propertyName) {
            continue;
        }
        if (ownerTypeName.empty() || ownerTypeName == property.OwnerTypeName) {
            return true;
        }
    }
    return false;
}

void AppendRawJsonLintDiagnosticsRecursive(
    const json& widgetJson,
    const std::string& rolePath,
    std::set<std::string>& seenEditorIds,
    FUiLintInfo& outInfo)
{
    if (widgetJson.is_null()) {
        return;
    }
    if (!widgetJson.is_object()) {
        outInfo.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "invalid_widget_node",
            "Widget node must be a JSON object.",
            std::string(),
            std::string(),
            rolePath));
        return;
    }

    const std::string typeName = widgetJson.value("Type", std::string());
    const std::string widgetId = widgetJson.value("EditorId", std::string());
    if (widgetId.empty()) {
        outInfo.Diagnostics.push_back(MakeLintDiagnostic(
            "warning",
            "missing_editor_id",
            "Widget is missing EditorId; the editor will assign one on save.",
            std::string(),
            typeName,
            rolePath));
    } else if (!seenEditorIds.insert(widgetId).second) {
        outInfo.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "duplicate_editor_id",
            "Duplicate EditorId found: " + widgetId,
            widgetId,
            typeName,
            rolePath));
    }

    FWidgetTypeInfo typeInfo;
    const bool bKnownType = WidgetCatalog::Get().TryDescribeWidgetType(typeName, typeInfo);
    if (typeName.empty()) {
        outInfo.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "missing_type",
            "Widget is missing Type.",
            widgetId,
            typeName,
            rolePath));
    } else if (!bKnownType) {
        outInfo.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "unknown_type",
            "Unsupported widget type: " + typeName,
            widgetId,
            typeName,
            rolePath));
    }

    if (widgetJson.contains("Properties")) {
        const json& properties = widgetJson["Properties"];
        if (!properties.is_object()) {
            outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                "error",
                "invalid_properties",
                "Properties must be a JSON object.",
                widgetId,
                typeName,
                "Properties"));
        } else if (bKnownType) {
            for (auto it = properties.begin(); it != properties.end(); ++it) {
                if (!IsKnownPropertyKey(typeInfo, it.key())) {
                    outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                        "warning",
                        "unknown_property",
                        "Property is not reflected on " + typeName + ": " + it.key(),
                        widgetId,
                        typeName,
                        it.key()));
                }
            }
        }
    }

    const auto appendChildArray = [&](const char* fieldName) {
        if (!widgetJson.contains(fieldName)) {
            return;
        }
        if (!widgetJson[fieldName].is_array()) {
            outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                "error",
                "invalid_child_array",
                std::string(fieldName) + " must be an array.",
                widgetId,
                typeName,
                fieldName));
            return;
        }

        const json& children = widgetJson[fieldName];
        for (std::size_t index = 0; index < children.size(); ++index) {
            AppendRawJsonLintDiagnosticsRecursive(
                children[index],
                std::string(fieldName) + "[" + std::to_string(index) + "]",
                seenEditorIds,
                outInfo);
        }
    };

    appendChildArray("Children");
    appendChildArray("LeadingItems");
    appendChildArray("TrailingItems");

    if (widgetJson.contains("Content")) {
        AppendRawJsonLintDiagnosticsRecursive(widgetJson["Content"], "Content", seenEditorIds, outInfo);
    }
    if (widgetJson.contains("Header")) {
        AppendRawJsonLintDiagnosticsRecursive(widgetJson["Header"], "Header", seenEditorIds, outInfo);
    }
    if (widgetJson.contains("Body")) {
        AppendRawJsonLintDiagnosticsRecursive(widgetJson["Body"], "Body", seenEditorIds, outInfo);
    }
    if (widgetJson.contains("TabItems")) {
        if (!widgetJson["TabItems"].is_array()) {
            outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                "error",
                "invalid_child_array",
                "TabItems must be an array.",
                widgetId,
                typeName,
                "TabItems"));
        } else {
            const json& tabItems = widgetJson["TabItems"];
            for (std::size_t index = 0; index < tabItems.size(); ++index) {
                if (!tabItems[index].is_object()) {
                    outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                        "error",
                        "invalid_tab_item",
                        "Tab item must be a JSON object.",
                        widgetId,
                        typeName,
                        "TabItems[" + std::to_string(index) + "]"));
                    continue;
                }
                if (tabItems[index].contains("Content")) {
                    AppendRawJsonLintDiagnosticsRecursive(
                        tabItems[index]["Content"],
                        "TabItems[" + std::to_string(index) + "].Content",
                        seenEditorIds,
                        outInfo);
                }
            }
        }
    }
}

void AppendTreeLintDiagnostics(const FUiDocumentTreeInfo& treeInfo, FUiLintInfo& outInfo)
{
    std::map<std::string, int> nonEmptyNameCounts;
    for (const FUiTreeNodeInfo& node : treeInfo.Nodes) {
        if (node.Name.empty()) {
            outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                "warning",
                "empty_name",
                "Widget name is empty, which makes generated code and agent edits harder to target.",
                node.WidgetId,
                node.TypeName,
                "Name"));
            continue;
        }
        ++nonEmptyNameCounts[node.Name];
    }

    for (const FUiTreeNodeInfo& node : treeInfo.Nodes) {
        if (!node.Name.empty() && nonEmptyNameCounts[node.Name] > 1) {
            outInfo.Diagnostics.push_back(MakeLintDiagnostic(
                "warning",
                "duplicate_name",
                "Widget name is duplicated: " + node.Name,
                node.WidgetId,
                node.TypeName,
                "Name"));
        }
    }
}

} // namespace

FUiDocumentTreeInfo BuildDocumentTreeInfo(const std::filesystem::path& inputPath)
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

FUiDocumentTreeInfo FindNodes(const std::filesystem::path& inputPath, const FUiFindRequest& request)
{
    FUiDocumentTreeInfo treeInfo = BuildDocumentTreeInfo(inputPath);
    if (!treeInfo.bSuccess) {
        return treeInfo;
    }

    FUiDocumentTreeInfo result;
    result.bSuccess = true;
    for (const FUiTreeNodeInfo& node : treeInfo.Nodes) {
        if (!request.WidgetId.empty() && node.WidgetId != request.WidgetId) {
            continue;
        }
        if (!request.TypeName.empty() && node.TypeName != request.TypeName) {
            continue;
        }
        if (!request.Name.empty() && node.Name != request.Name) {
            continue;
        }
        result.Nodes.push_back(node);
    }
    return result;
}

FUiNodeInspectInfo InspectNode(const std::filesystem::path& inputPath, const std::string& widgetId)
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

    const std::size_t depth = CalculateWidgetDepth(document, widget);
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

FUiLintInfo LintDocumentFile(const std::filesystem::path& inputPath)
{
    FUiLintInfo result;

    json documentJson;
    try {
        std::ifstream stream(inputPath);
        if (!stream.is_open()) {
            result.ErrorMessage = "Failed to open file for reading: " + inputPath.string();
            return result;
        }
        stream >> documentJson;
    } catch (const std::exception& exception) {
        result.ErrorMessage = exception.what();
        return result;
    }

    if (!documentJson.is_object()) {
        result.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "invalid_document",
            "Document JSON must be an object."));
        result.bSuccess = true;
        return result;
    }

    if (documentJson.value("Format", std::string()) != "ImWidgetV4EditorDocument") {
        result.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "unsupported_format",
            "Unsupported document format."));
    }
    if (documentJson.value("Version", 0) != kEditorDocumentFormatVersion) {
        result.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "unsupported_version",
            "Unsupported document version."));
    }
    if (!documentJson.contains("RootWidget")) {
        result.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "missing_root",
            "Document JSON is missing RootWidget."));
    } else {
        std::set<std::string> seenEditorIds;
        AppendRawJsonLintDiagnosticsRecursive(documentJson["RootWidget"], "RootWidget", seenEditorIds, result);
    }

    EditorDocument document;
    std::string loadError;
    if (!document.Load(inputPath, &loadError)) {
        result.Diagnostics.push_back(MakeLintDiagnostic(
            "error",
            "load_failed",
            "EditorDocument failed to load the document: " + loadError));
        result.bSuccess = true;
        return result;
    }

    FUiDocumentTreeInfo treeInfo;
    treeInfo.bSuccess = true;
    AppendTreeNode(document.GetRootWidget(), document, 0, treeInfo);
    AppendTreeLintDiagnostics(treeInfo, result);

    result.bSuccess = true;
    return result;
}

} // namespace ImWidgetV4Editor
