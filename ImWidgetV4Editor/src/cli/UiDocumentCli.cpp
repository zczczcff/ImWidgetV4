#include "UiDocumentCli.h"

#include "../editor/DocumentEditService.h"
#include "../editor/LogicalWidgetTree.h"
#include "../serialization/WidgetFactory.h"

#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <imwidgetv4/widgets/Image.h>

#include <algorithm>
#include <fstream>
#include <map>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

struct FUiDiffSnapshotNode {
    FUiTreeNodeInfo Node;
    json Properties = json::object();
};

struct FUiDiffSnapshot {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::map<std::string, FUiDiffSnapshotNode> Nodes;
};

void AppendDiffSnapshotNode(
    const std::shared_ptr<ImWidget>& widget,
    EditorDocument& document,
    std::size_t depth,
    FUiDiffSnapshot& outSnapshot)
{
    if (!widget) {
        return;
    }

    FUiDiffSnapshotNode snapshotNode;
    snapshotNode.Node = BuildTreeNodeInfo(widget, document, depth);
    json widgetJson = widget->ToJson();
    if (widgetJson.is_object() && widgetJson.contains("Properties") && widgetJson["Properties"].is_object()) {
        snapshotNode.Properties = widgetJson["Properties"];
    }
    outSnapshot.Nodes[snapshotNode.Node.WidgetId] = std::move(snapshotNode);

    const std::size_t childCount = LogicalWidgetTree::GetLogicalChildCount(widget);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        AppendDiffSnapshotNode(LogicalWidgetTree::GetLogicalChildAt(widget, childIndex), document, depth + 1, outSnapshot);
    }
}

FUiDiffSnapshot BuildDiffSnapshot(const std::filesystem::path& inputPath)
{
    FUiDiffSnapshot snapshot;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        snapshot.ErrorMessage = error;
        return snapshot;
    }

    snapshot.bSuccess = true;
    AppendDiffSnapshotNode(document.GetRootWidget(), document, 0, snapshot);
    return snapshot;
}

bool HasNodeIdentityChange(const FUiTreeNodeInfo& beforeNode, const FUiTreeNodeInfo& afterNode)
{
    return beforeNode.ParentWidgetId != afterNode.ParentWidgetId ||
        beforeNode.TypeName != afterNode.TypeName ||
        beforeNode.Name != afterNode.Name ||
        beforeNode.RoleName != afterNode.RoleName ||
        beforeNode.CodegenMemberAccess != afterNode.CodegenMemberAccess ||
        beforeNode.Depth != afterNode.Depth ||
        beforeNode.ChildIndex != afterNode.ChildIndex;
}

void AppendNodeFieldDiffs(
    const FUiTreeNodeInfo& beforeNode,
    const FUiTreeNodeInfo& afterNode,
    FUiDocumentDiffInfo& outDiff)
{
    const auto appendField = [&](const std::string& fieldName, const json& beforeValue, const json& afterValue) {
        if (beforeValue == afterValue) {
            return;
        }

        FUiNodeDiffEntry entry;
        entry.Kind = "changed";
        entry.WidgetId = beforeNode.WidgetId;
        entry.FieldName = fieldName;
        entry.BeforeNode = beforeNode;
        entry.AfterNode = afterNode;
        entry.BeforeValue = beforeValue;
        entry.AfterValue = afterValue;
        outDiff.Entries.push_back(std::move(entry));
    };

    appendField("parent", beforeNode.ParentWidgetId, afterNode.ParentWidgetId);
    appendField("type", beforeNode.TypeName, afterNode.TypeName);
    appendField("name", beforeNode.Name, afterNode.Name);
    appendField("role", beforeNode.RoleName, afterNode.RoleName);
    appendField("codegenMemberAccess", beforeNode.CodegenMemberAccess, afterNode.CodegenMemberAccess);
    appendField("depth", beforeNode.Depth, afterNode.Depth);
    appendField("index", beforeNode.ChildIndex, afterNode.ChildIndex);
}

void AppendPropertyDiffs(
    const FUiDiffSnapshotNode& beforeNode,
    const FUiDiffSnapshotNode& afterNode,
    FUiDocumentDiffInfo& outDiff)
{
    std::map<std::string, bool> propertyKeys;
    if (beforeNode.Properties.is_object()) {
        for (auto it = beforeNode.Properties.begin(); it != beforeNode.Properties.end(); ++it) {
            propertyKeys[it.key()] = true;
        }
    }
    if (afterNode.Properties.is_object()) {
        for (auto it = afterNode.Properties.begin(); it != afterNode.Properties.end(); ++it) {
            propertyKeys[it.key()] = true;
        }
    }

    for (const auto& propertyKey : propertyKeys) {
        const json beforeValue =
            beforeNode.Properties.contains(propertyKey.first) ? beforeNode.Properties.at(propertyKey.first) : json();
        const json afterValue =
            afterNode.Properties.contains(propertyKey.first) ? afterNode.Properties.at(propertyKey.first) : json();
        if (beforeValue == afterValue) {
            continue;
        }

        FUiNodeDiffEntry entry;
        entry.Kind = "property";
        entry.WidgetId = beforeNode.Node.WidgetId;
        entry.FieldName = propertyKey.first;
        entry.BeforeNode = beforeNode.Node;
        entry.AfterNode = afterNode.Node;
        entry.BeforeValue = beforeValue;
        entry.AfterValue = afterValue;
        outDiff.Entries.push_back(std::move(entry));
    }
}

FUiPatchOperationResult MakePatchFailure(
    const std::string& operation,
    const std::string& message)
{
    FUiPatchOperationResult result;
    result.Operation = operation;
    result.ErrorMessage = message;
    return result;
}

bool ApplyJsonPropertiesToWidget(
    const std::shared_ptr<ImWidget>& widget,
    const json& properties,
    std::string& outError)
{
    if (!widget) {
        outError = "Widget is required.";
        return false;
    }
    if (!properties.is_object()) {
        outError = "properties must be a JSON object.";
        return false;
    }

    json beforeJson = widget->ToJson();
    json afterJson = beforeJson;
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        std::string ownerTypeName;
        std::string propertyName = it.key();
        const std::size_t separator = propertyName.find("::");
        if (separator != std::string::npos) {
            ownerTypeName = propertyName.substr(0, separator);
            propertyName = propertyName.substr(separator + 2);
        }

        const Reflection::FPropertyDesc* property =
            Reflection::FindProperty(widget->GetTypeDesc(), propertyName, ownerTypeName);
        if (!property) {
            outError = "Property was not found on " + widget->GetTypeName() + ": " + it.key();
            return false;
        }
        afterJson["Properties"][std::string(property->OwnerTypeName) + "::" + property->Name] = it.value();
    }

    try {
        widget->FromJson(afterJson);
    } catch (const std::exception& exception) {
        outError = exception.what();
        return false;
    }
    return true;
}

bool ApplySinglePatchOperation(
    EditorDocument& document,
    const json& operationJson,
    FUiPatchOperationResult& outResult)
{
    if (!operationJson.is_object()) {
        outResult = MakePatchFailure(std::string(), "Patch operation must be a JSON object.");
        return false;
    }

    const std::string operation = operationJson.value("op", operationJson.value("operation", std::string()));
    outResult.Operation = operation;
    if (operation.empty()) {
        outResult.ErrorMessage = "Patch operation is missing op.";
        return false;
    }

    if (operation == "rename") {
        const std::string widgetId = operationJson.value("id", std::string());
        const std::string name = operationJson.value("name", std::string());
        std::shared_ptr<ImWidget> widget = document.FindWidgetById(widgetId);
        if (!widget) {
            outResult.ErrorMessage = "Widget id was not found: " + widgetId;
            return false;
        }

        outResult.bChanged = widget->GetName() != name;
        widget->SetName(name);
        outResult.Node = BuildTreeNodeInfo(widget, document, CalculateWidgetDepth(document, widget));
        outResult.bSuccess = true;
        return true;
    }

    if (operation == "set") {
        const std::string widgetId = operationJson.value("id", std::string());
        const std::string propertyName = operationJson.value("property", std::string());
        if (!operationJson.contains("value")) {
            outResult.ErrorMessage = "set operation requires value.";
            return false;
        }

        std::shared_ptr<ImWidget> widget = document.FindWidgetById(widgetId);
        if (!widget) {
            outResult.ErrorMessage = "Widget id was not found: " + widgetId;
            return false;
        }

        const Reflection::FPropertyDesc* property =
            Reflection::FindProperty(widget->GetTypeDesc(), propertyName);
        if (!property) {
            outResult.ErrorMessage = "Property was not found on " + widget->GetTypeName() + ": " + propertyName;
            return false;
        }

        const json beforeJson = widget->ToJson();
        json afterJson = beforeJson;
        afterJson["Properties"][std::string(property->OwnerTypeName) + "::" + property->Name] = operationJson["value"];
        try {
            widget->FromJson(afterJson);
        } catch (const std::exception& exception) {
            outResult.ErrorMessage = exception.what();
            return false;
        }

        outResult.bChanged = beforeJson != widget->ToJson();
        outResult.Node = BuildTreeNodeInfo(widget, document, CalculateWidgetDepth(document, widget));
        outResult.bSuccess = true;
        return true;
    }

    if (operation == "add") {
        const std::string parentId = operationJson.value("parent", operationJson.value("parentId", std::string()));
        const std::string typeName = operationJson.value("type", std::string());
        std::shared_ptr<ImWidget> parent = document.FindWidgetById(parentId);
        if (!parent) {
            outResult.ErrorMessage = "Parent widget id was not found: " + parentId;
            return false;
        }

        std::shared_ptr<ImWidget> child = WidgetFactory::Get().CreateWidget(typeName);
        if (!child) {
            outResult.ErrorMessage = "Unsupported widget type: " + typeName;
            return false;
        }
        InitializeNewWidgetDefaults(child);
        if (operationJson.contains("name")) {
            child->SetName(operationJson.value("name", std::string()));
        }
        if (operationJson.contains("properties")) {
            std::string propertyError;
            if (!ApplyJsonPropertiesToWidget(child, operationJson["properties"], propertyError)) {
                outResult.ErrorMessage = propertyError;
                return false;
            }
        }

        std::string insertError;
        if (!TryInsertWidgetIntoParent(parent, child, insertError)) {
            outResult.ErrorMessage = insertError;
            return false;
        }

        outResult.bChanged = true;
        outResult.Node = BuildTreeNodeInfo(child, document, CalculateWidgetDepth(document, child));
        outResult.bSuccess = true;
        return true;
    }

    if (operation == "remove") {
        const std::string widgetId = operationJson.value("id", std::string());
        std::shared_ptr<ImWidget> widget = document.FindWidgetById(widgetId);
        if (!widget) {
            outResult.ErrorMessage = "Widget id was not found: " + widgetId;
            return false;
        }
        if (widget == document.GetRootWidget()) {
            outResult.ErrorMessage = "Removing the root widget is not supported by ui patch.";
            return false;
        }

        std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
        outResult.Node = BuildTreeNodeInfo(widget, document, CalculateWidgetDepth(document, widget));
        if (!RemoveWidgetFromParent(parent, widget)) {
            outResult.ErrorMessage = "Failed to remove widget from parent.";
            return false;
        }

        outResult.bChanged = true;
        outResult.bSuccess = true;
        return true;
    }

    if (operation == "duplicate") {
        const std::string widgetId = operationJson.value("id", std::string());
        std::shared_ptr<ImWidget> source = document.FindWidgetById(widgetId);
        if (!source) {
            outResult.ErrorMessage = "Widget id was not found: " + widgetId;
            return false;
        }
        if (source == document.GetRootWidget()) {
            outResult.ErrorMessage = "Duplicating the root widget is not supported by ui patch.";
            return false;
        }

        std::shared_ptr<ImWidget> parent = document.FindLogicalParent(source);
        if (!parent) {
            outResult.ErrorMessage = "Source widget has no logical parent.";
            return false;
        }

        std::string cloneError;
        std::shared_ptr<ImWidget> cloneWidget = CloneWidgetTree(source, cloneError);
        if (!cloneWidget) {
            outResult.ErrorMessage = cloneError;
            return false;
        }
        if (operationJson.contains("name")) {
            cloneWidget->SetName(operationJson.value("name", std::string()));
        } else if (!cloneWidget->GetName().empty()) {
            cloneWidget->SetName(cloneWidget->GetName() + "Copy");
        }

        std::string insertError;
        if (!TryInsertWidgetIntoParent(parent, cloneWidget, insertError)) {
            outResult.ErrorMessage = insertError;
            return false;
        }

        outResult.bChanged = true;
        outResult.Node = BuildTreeNodeInfo(cloneWidget, document, CalculateWidgetDepth(document, cloneWidget));
        outResult.bSuccess = true;
        return true;
    }

    if (operation == "move") {
        const std::string widgetId = operationJson.value("id", std::string());
        const std::string parentId = operationJson.value("parent", operationJson.value("parentId", std::string()));
        std::shared_ptr<ImWidget> widget = document.FindWidgetById(widgetId);
        if (!widget) {
            outResult.ErrorMessage = "Widget id was not found: " + widgetId;
            return false;
        }
        if (widget == document.GetRootWidget()) {
            outResult.ErrorMessage = "Moving the root widget is not supported by ui patch.";
            return false;
        }

        std::shared_ptr<ImWidget> oldParent = document.FindLogicalParent(widget);
        std::shared_ptr<ImWidget> newParent = document.FindWidgetById(parentId);
        if (!oldParent) {
            outResult.ErrorMessage = "Widget has no logical parent.";
            return false;
        }
        if (!newParent) {
            outResult.ErrorMessage = "New parent widget id was not found: " + parentId;
            return false;
        }
        if (newParent == widget || IsLogicalAncestorOf(document, widget, newParent)) {
            outResult.ErrorMessage = "Cannot move a widget into itself or one of its descendants.";
            return false;
        }
        if (oldParent == newParent) {
            outResult.Node = BuildTreeNodeInfo(widget, document, CalculateWidgetDepth(document, widget));
            outResult.bSuccess = true;
            return true;
        }

        if (!RemoveWidgetFromParent(oldParent, widget)) {
            outResult.ErrorMessage = "Failed to detach widget from old parent.";
            return false;
        }

        std::string insertError;
        if (!TryInsertWidgetIntoParent(newParent, widget, insertError)) {
            std::string restoreError;
            (void)TryInsertWidgetIntoParent(oldParent, widget, restoreError);
            outResult.ErrorMessage = insertError;
            return false;
        }

        outResult.bChanged = true;
        outResult.Node = BuildTreeNodeInfo(widget, document, CalculateWidgetDepth(document, widget));
        outResult.bSuccess = true;
        return true;
    }

    outResult.ErrorMessage = "Unsupported patch operation: " + operation;
    return false;
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

FUiMutationResult UiDocumentCli::FormatDocumentFile(const std::filesystem::path& inputPath)
{
    FUiMutationResult result;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    const json beforeJson = document.ExportDocumentJson();
    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    result.bChanged = beforeJson != document.ExportDocumentJson();
    if (document.GetRootWidget()) {
        result.Node = BuildTreeNodeInfo(document.GetRootWidget(), document, 0);
    }
    return result;
}

FUiPatchResult UiDocumentCli::PatchDocumentFile(
    const std::filesystem::path& inputPath,
    const std::filesystem::path& patchPath)
{
    FUiPatchResult result;

    json patchJson;
    try {
        std::ifstream stream(patchPath);
        if (!stream.is_open()) {
            result.ErrorMessage = "Failed to open patch file for reading: " + patchPath.string();
            return result;
        }
        stream >> patchJson;
    } catch (const std::exception& exception) {
        result.ErrorMessage = exception.what();
        return result;
    }

    json operationsJson;
    if (patchJson.is_array()) {
        operationsJson = patchJson;
    } else if (patchJson.is_object() && patchJson.contains("operations")) {
        operationsJson = patchJson["operations"];
    } else {
        result.ErrorMessage = "Patch JSON must be an array or an object with operations.";
        return result;
    }

    if (!operationsJson.is_array()) {
        result.ErrorMessage = "Patch operations must be a JSON array.";
        return result;
    }

    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    const json beforeJson = document.ExportDocumentJson();
    for (const json& operationJson : operationsJson) {
        FUiPatchOperationResult operationResult;
        if (!ApplySinglePatchOperation(document, operationJson, operationResult)) {
            result.Operations.push_back(operationResult);
            result.ErrorMessage = operationResult.ErrorMessage;
            return result;
        }
        result.Operations.push_back(operationResult);
    }

    result.bChanged = beforeJson != document.ExportDocumentJson();
    document.SetDirty(result.bChanged);
    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    return result;
}

FUiDocumentTreeInfo UiDocumentCli::BuildDocumentTreeInfo(const std::filesystem::path& inputPath)
{
    return ImWidgetV4Editor::BuildDocumentTreeInfo(inputPath);
}

FUiDocumentTreeInfo UiDocumentCli::FindNodes(const std::filesystem::path& inputPath, const FUiFindRequest& request)
{
    return ImWidgetV4Editor::FindNodes(inputPath, request);
}

FUiNodeInspectInfo UiDocumentCli::InspectNode(const std::filesystem::path& inputPath, const std::string& widgetId)
{
    return ImWidgetV4Editor::InspectNode(inputPath, widgetId);
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

FUiMutationResult UiDocumentCli::SetNodeCodegenMemberAccess(
    const std::filesystem::path& inputPath,
    const std::string& widgetId,
    const std::string& memberAccess)
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

    std::string normalizedAccess = memberAccess;
    std::transform(
        normalizedAccess.begin(),
        normalizedAccess.end(),
        normalizedAccess.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalizedAccess == "public") {
        normalizedAccess = kEditorCodegenMemberAccessPublic;
    } else if (normalizedAccess == "private") {
        normalizedAccess = kEditorCodegenMemberAccessPrivate;
    } else {
        result.ErrorMessage = "Codegen member access must be public or private.";
        return result;
    }

    json metadata = document.GetWidgetCodegenMetadata(widget);
    const std::string beforeAccess = metadata.value(
        kEditorCodegenMemberAccessFieldName,
        std::string(kEditorCodegenMemberAccessPublic));
    result.bChanged = beforeAccess != normalizedAccess;
    if (normalizedAccess == kEditorCodegenMemberAccessPublic) {
        metadata.erase(kEditorCodegenMemberAccessFieldName);
    } else {
        metadata[kEditorCodegenMemberAccessFieldName] = normalizedAccess;
    }
    document.SetWidgetCodegenMetadata(widget, metadata);

    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.Node = BuildTreeNodeInfo(widget, document, CalculateWidgetDepth(document, widget));
    result.bSuccess = true;
    return result;
}

FUiMutationResult UiDocumentCli::SetNodeProperty(
    const std::filesystem::path& inputPath,
    const std::string& widgetId,
    const std::string& propertyName,
    const json& value)
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

    const Reflection::FPropertyDesc* property =
        Reflection::FindProperty(widget->GetTypeDesc(), propertyName);
    if (!property) {
        result.ErrorMessage = "Property was not found on " + widget->GetTypeName() + ": " + propertyName;
        return result;
    }

    const json beforeJson = widget->ToJson();
    json afterJson = beforeJson;
    afterJson["Properties"][std::string(property->OwnerTypeName) + "::" + property->Name] = value;

    try {
        widget->FromJson(afterJson);
    } catch (const std::exception& exception) {
        result.ErrorMessage = exception.what();
        return result;
    }

    result.bChanged = beforeJson != widget->ToJson();
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

FUiMutationResult UiDocumentCli::AddNode(
    const std::filesystem::path& inputPath,
    const std::string& parentWidgetId,
    const std::string& widgetTypeName)
{
    FUiMutationResult result;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    const std::shared_ptr<ImWidget> parent = document.FindWidgetById(parentWidgetId);
    if (!parent) {
        result.ErrorMessage = "Parent widget id was not found: " + parentWidgetId;
        return result;
    }

    std::shared_ptr<ImWidget> child = WidgetFactory::Get().CreateWidget(widgetTypeName);
    if (!child) {
        result.ErrorMessage = "Unsupported widget type: " + widgetTypeName;
        return result;
    }
    InitializeNewWidgetDefaults(child);

    if (!TryInsertWidgetIntoParent(parent, child, error)) {
        result.ErrorMessage = error;
        return result;
    }

    document.SetDirty(true);
    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    result.bChanged = true;
    result.Node = BuildTreeNodeInfo(child, document, 0);
    if (const std::shared_ptr<ImWidget> reloadedChild = document.FindWidgetById(result.Node.WidgetId)) {
        std::size_t depth = 0;
        std::shared_ptr<ImWidget> nodeParent = document.FindLogicalParent(reloadedChild);
        while (nodeParent) {
            ++depth;
            nodeParent = document.FindLogicalParent(nodeParent);
        }
        result.Node = BuildTreeNodeInfo(reloadedChild, document, depth);
    }
    return result;
}

FUiMutationResult UiDocumentCli::RemoveNode(
    const std::filesystem::path& inputPath,
    const std::string& widgetId)
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
    if (widget == document.GetRootWidget()) {
        result.ErrorMessage = "Removing the root widget is not supported by ui remove.";
        return result;
    }

    std::size_t depth = 0;
    std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
    std::shared_ptr<ImWidget> depthParent = parent;
    while (depthParent) {
        ++depth;
        depthParent = document.FindLogicalParent(depthParent);
    }
    result.Node = BuildTreeNodeInfo(widget, document, depth);

    if (!RemoveWidgetFromParent(parent, widget)) {
        result.ErrorMessage = "Failed to remove widget from parent.";
        return result;
    }

    document.SetDirty(true);
    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    result.bChanged = true;
    return result;
}

FUiMutationResult UiDocumentCli::DuplicateNode(
    const std::filesystem::path& inputPath,
    const std::string& widgetId)
{
    FUiMutationResult result;
    EditorDocument document;
    std::string error;
    if (!document.Load(inputPath, &error)) {
        result.ErrorMessage = error;
        return result;
    }

    const std::shared_ptr<ImWidget> source = document.FindWidgetById(widgetId);
    if (!source) {
        result.ErrorMessage = "Widget id was not found: " + widgetId;
        return result;
    }
    if (source == document.GetRootWidget()) {
        result.ErrorMessage = "Duplicating the root widget is not supported by ui duplicate.";
        return result;
    }

    const std::shared_ptr<ImWidget> parent = document.FindLogicalParent(source);
    if (!parent) {
        result.ErrorMessage = "Source widget has no logical parent.";
        return result;
    }

    std::string cloneError;
    std::shared_ptr<ImWidget> cloneWidget = CloneWidgetTree(source, cloneError);
    if (!cloneWidget) {
        result.ErrorMessage = cloneError;
        return result;
    }

    if (!cloneWidget->GetName().empty()) {
        cloneWidget->SetName(cloneWidget->GetName() + "Copy");
    }

    if (!TryDuplicateWidgetInParent(parent, source, cloneWidget, error)) {
        result.ErrorMessage = error;
        return result;
    }

    document.SetDirty(true);
    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    result.bChanged = true;
    result.Node = BuildTreeNodeInfo(cloneWidget, document, 0);
    if (const std::shared_ptr<ImWidget> reloadedClone = document.FindWidgetById(result.Node.WidgetId)) {
        std::size_t depth = 0;
        std::shared_ptr<ImWidget> nodeParent = document.FindLogicalParent(reloadedClone);
        while (nodeParent) {
            ++depth;
            nodeParent = document.FindLogicalParent(nodeParent);
        }
        result.Node = BuildTreeNodeInfo(reloadedClone, document, depth);
    }
    return result;
}

FUiMutationResult UiDocumentCli::MoveNode(
    const std::filesystem::path& inputPath,
    const std::string& widgetId,
    const std::string& newParentWidgetId)
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
    if (widget == document.GetRootWidget()) {
        result.ErrorMessage = "Moving the root widget is not supported by ui move.";
        return result;
    }

    const std::shared_ptr<ImWidget> oldParent = document.FindLogicalParent(widget);
    if (!oldParent) {
        result.ErrorMessage = "Widget has no logical parent.";
        return result;
    }

    const std::shared_ptr<ImWidget> newParent = document.FindWidgetById(newParentWidgetId);
    if (!newParent) {
        result.ErrorMessage = "New parent widget id was not found: " + newParentWidgetId;
        return result;
    }
    if (newParent == widget || IsLogicalAncestorOf(document, widget, newParent)) {
        result.ErrorMessage = "Cannot move a widget into itself or one of its descendants.";
        return result;
    }

    if (oldParent == newParent) {
        std::size_t depth = 0;
        std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
        while (parent) {
            ++depth;
            parent = document.FindLogicalParent(parent);
        }
        result.bSuccess = true;
        result.bChanged = false;
        result.Node = BuildTreeNodeInfo(widget, document, depth);
        return result;
    }

    if (!RemoveWidgetFromParent(oldParent, widget)) {
        result.ErrorMessage = "Failed to detach widget from old parent.";
        return result;
    }

    if (!TryInsertWidgetIntoParent(newParent, widget, error)) {
        std::string restoreError;
        (void)TryInsertWidgetIntoParent(oldParent, widget, restoreError);
        result.ErrorMessage = error;
        return result;
    }

    document.SetDirty(true);
    if (!document.Save(&error)) {
        result.ErrorMessage = error;
        return result;
    }

    result.bSuccess = true;
    result.bChanged = true;
    result.Node = BuildTreeNodeInfo(widget, document, 0);
    if (const std::shared_ptr<ImWidget> movedWidget = document.FindWidgetById(result.Node.WidgetId)) {
        std::size_t depth = 0;
        std::shared_ptr<ImWidget> parent = document.FindLogicalParent(movedWidget);
        while (parent) {
            ++depth;
            parent = document.FindLogicalParent(parent);
        }
        result.Node = BuildTreeNodeInfo(movedWidget, document, depth);
    }
    return result;
}

FUiDocumentDiffInfo UiDocumentCli::DiffDocuments(
    const std::filesystem::path& beforePath,
    const std::filesystem::path& afterPath)
{
    FUiDocumentDiffInfo result;
    const FUiDiffSnapshot beforeSnapshot = BuildDiffSnapshot(beforePath);
    if (!beforeSnapshot.bSuccess) {
        result.ErrorMessage = "Failed to load before UI document: " + beforeSnapshot.ErrorMessage;
        return result;
    }

    const FUiDiffSnapshot afterSnapshot = BuildDiffSnapshot(afterPath);
    if (!afterSnapshot.bSuccess) {
        result.ErrorMessage = "Failed to load after UI document: " + afterSnapshot.ErrorMessage;
        return result;
    }

    for (const auto& beforePair : beforeSnapshot.Nodes) {
        if (afterSnapshot.Nodes.find(beforePair.first) != afterSnapshot.Nodes.end()) {
            continue;
        }

        FUiNodeDiffEntry entry;
        entry.Kind = "removed";
        entry.WidgetId = beforePair.first;
        entry.BeforeNode = beforePair.second.Node;
        entry.BeforeValue = beforePair.second.Properties;
        result.Entries.push_back(std::move(entry));
    }

    for (const auto& afterPair : afterSnapshot.Nodes) {
        const auto beforeIt = beforeSnapshot.Nodes.find(afterPair.first);
        if (beforeIt == beforeSnapshot.Nodes.end()) {
            FUiNodeDiffEntry entry;
            entry.Kind = "added";
            entry.WidgetId = afterPair.first;
            entry.AfterNode = afterPair.second.Node;
            entry.AfterValue = afterPair.second.Properties;
            result.Entries.push_back(std::move(entry));
            continue;
        }

        if (HasNodeIdentityChange(beforeIt->second.Node, afterPair.second.Node)) {
            AppendNodeFieldDiffs(beforeIt->second.Node, afterPair.second.Node, result);
        }
        AppendPropertyDiffs(beforeIt->second, afterPair.second, result);
    }

    result.bSuccess = true;
    result.bChanged = !result.Entries.empty();
    return result;
}

FUiLintInfo UiDocumentCli::LintDocumentFile(const std::filesystem::path& inputPath)
{
    return ImWidgetV4Editor::LintDocumentFile(inputPath);
}

} // namespace ImWidgetV4Editor
