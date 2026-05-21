#include "UiDocumentCli.h"

#include "../editor/LogicalWidgetTree.h"
#include "../serialization/WidgetCatalog.h"
#include "../serialization/WidgetFactory.h"
#include "../serialization/WidgetSerializer.h"

#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4/widgets/VerticalBox.h>

#include <fstream>
#include <map>
#include <set>

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

FUiLintDiagnostic MakeLintDiagnostic(
    const std::string& severity,
    const std::string& code,
    const std::string& message,
    const std::string& widgetId = std::string(),
    const std::string& typeName = std::string(),
    const std::string& fieldName = std::string())
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

std::string BuildDefaultWidgetName(const std::string& typeName)
{
    std::string name = typeName;
    if (name.size() > 2 && name[0] == 'I' && name[1] == 'm') {
        name = name.substr(2);
    }
    return name.empty() ? "Widget" : name;
}

std::string BuildTabTitleForWidget(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return "Tab";
    }
    return widget->GetName().empty() ? widget->GetTypeName() : widget->GetName();
}

void InitializeNewWidgetDefaults(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return;
    }

    widget->SetName(BuildDefaultWidgetName(widget->GetTypeName()));
    if (auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(widget)) {
        textBlock->SetText("Text");
    } else if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        button->SetText("Button");
    } else if (auto comboBox = std::dynamic_pointer_cast<ImComboBox>(widget)) {
        comboBox->SetItems({"Option A", "Option B", "Option C"});
        comboBox->SetSelectedIndex(0);
    } else if (auto checkBox = std::dynamic_pointer_cast<ImCheckBox>(widget)) {
        checkBox->SetLabel("CheckBox");
    } else if (auto editableText = std::dynamic_pointer_cast<ImEditableText>(widget)) {
        editableText->SetText("EditableText");
    } else if (auto textList = std::dynamic_pointer_cast<ImTextList>(widget)) {
        textList->SetItems({"Item 1", "Item 2", "Item 3"});
    } else if (auto colorPicker = std::dynamic_pointer_cast<ImColorPicker>(widget)) {
        colorPicker->SetColor(FColor::FromBytes(86, 156, 214));
    } else if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        auto content = std::make_shared<ImTextBlock>();
        content->SetName("TabContent");
        content->SetText("Tab Content");
        tabView->AddTab("Tab", content);
    } else if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        auto header = std::make_shared<ImTextBlock>();
        header->SetName("Header");
        header->SetText("Expandable Header");
        auto body = std::make_shared<ImTextBlock>();
        body->SetName("Body");
        body->SetText("Expandable Body");
        expandableBox->SetHeader(header);
        expandableBox->SetBody(body);
        expandableBox->SetExpanded(true);
    }
}

bool InsertWidgetIntoParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& child,
    std::string& outError)
{
    if (!parent || !child) {
        outError = "Parent and child widgets are required.";
        return false;
    }

    if (auto canvas = std::dynamic_pointer_cast<ImCanvasPanel>(parent)) {
        canvas->AddChildAt(child, FVector2(0.05f, 0.05f));
        return true;
    }
    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        verticalBox->AddChild(child);
        return true;
    }
    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        horizontalBox->AddChild(child);
        return true;
    }
    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        if (button->GetContent()) {
            outError = "Button already has content.";
            return false;
        }
        button->SetContent(child);
        return true;
    }
    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        if (!scrollBox->GetContent()) {
            scrollBox->SetContent(child);
            return true;
        }
        if (auto contentVerticalBox = std::dynamic_pointer_cast<ImVerticalBox>(scrollBox->GetContent())) {
            contentVerticalBox->AddChild(child);
            return true;
        }
        auto wrapper = std::make_shared<ImVerticalBox>();
        wrapper->SetName("ScrollContent");
        wrapper->AddChild(scrollBox->GetContent());
        wrapper->AddChild(child);
        scrollBox->SetContent(wrapper);
        return true;
    }
    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (!expandableBox->GetHeader()) {
            expandableBox->SetHeader(child);
            return true;
        }
        if (!expandableBox->GetBody()) {
            expandableBox->SetBody(child);
            expandableBox->SetExpanded(true);
            return true;
        }
        outError = "ExpandableBox already has header and body.";
        return false;
    }
    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int tabIndex = tabView->AddTab(BuildTabTitleForWidget(child), child);
        if (tabIndex < 0) {
            outError = "Failed to add tab content.";
            return false;
        }
        tabView->SetActiveTab(tabIndex);
        return true;
    }
    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        titleBar->AddLeadingItem(child);
        return true;
    }
    if (auto userWidget = std::dynamic_pointer_cast<ImUserWidget>(parent)) {
        if (userWidget->GetRootWidget()) {
            outError = "UserWidget already has a root widget.";
            return false;
        }
        userWidget->SetRootWidget(child);
        return true;
    }

    outError = "Parent widget does not accept child widgets: " + parent->GetTypeName();
    return false;
}

bool RemoveWidgetFromParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& child)
{
    if (!parent || !child) {
        return false;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        const int tabIndex = LogicalWidgetTree::FindTabContentIndex(tabView, child);
        return tabIndex >= 0 && tabView->RemoveTab(tabIndex);
    }
    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        return titleBar->RemoveLeadingItem(child) || titleBar->RemoveTrailingItem(child);
    }
    if (auto userWidget = std::dynamic_pointer_cast<ImUserWidget>(parent)) {
        if (userWidget->GetRootWidget() == child) {
            userWidget->SetRootWidget(nullptr);
            return true;
        }
    }
    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        if (button->GetContent() == child) {
            button->SetContent(nullptr);
            return true;
        }
    }
    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (expandableBox->GetHeader() == child) {
            expandableBox->SetHeader(nullptr);
            return true;
        }
        if (expandableBox->GetBody() == child) {
            expandableBox->SetBody(nullptr);
            return true;
        }
    }
    if (auto panelParent = std::dynamic_pointer_cast<ImPanelWidget>(parent)) {
        return panelParent->RemoveChild(child);
    }

    return parent->RemoveChild(child);
}

bool IsLogicalAncestorOf(
    EditorDocument& document,
    const std::shared_ptr<ImWidget>& possibleAncestor,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!possibleAncestor || !widget) {
        return false;
    }

    std::shared_ptr<ImWidget> parent = document.FindLogicalParent(widget);
    while (parent) {
        if (parent == possibleAncestor) {
            return true;
        }
        parent = document.FindLogicalParent(parent);
    }
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

FUiDocumentTreeInfo UiDocumentCli::FindNodes(const std::filesystem::path& inputPath, const FUiFindRequest& request)
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

    if (!InsertWidgetIntoParent(parent, child, error)) {
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

    FWidgetSerializationResult cloneResult =
        WidgetSerializer::DeserializeWidgetTree(WidgetSerializer::SerializeWidgetTree(source));
    if (!cloneResult.bSuccess || !cloneResult.Widget) {
        result.ErrorMessage = cloneResult.ErrorMessage.empty()
            ? "Failed to clone widget."
            : cloneResult.ErrorMessage;
        return result;
    }

    if (!cloneResult.Widget->GetName().empty()) {
        cloneResult.Widget->SetName(cloneResult.Widget->GetName() + "Copy");
    }

    if (!InsertWidgetIntoParent(parent, cloneResult.Widget, error)) {
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
    result.Node = BuildTreeNodeInfo(cloneResult.Widget, document, 0);
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

    if (!InsertWidgetIntoParent(newParent, widget, error)) {
        std::string restoreError;
        (void)InsertWidgetIntoParent(oldParent, widget, restoreError);
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
