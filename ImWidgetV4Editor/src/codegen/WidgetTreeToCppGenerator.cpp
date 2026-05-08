#include "WidgetTreeToCppGenerator.h"

#include "../serialization/WidgetSerializer.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

enum class EGeneratedChildRelation {
    GenericChild,
    ButtonContent,
    ScrollContent,
    ExpandableHeader,
    ExpandableBody,
    TabContent
};

struct FTabRelationData {
    int Index = -1;
    std::string Title;
    bool bEnabled = true;
    bool bClosable = false;
    bool bDirty = false;
};

struct FGeneratedNode {
    int Index = -1;
    int ParentIndex = -1;
    std::string TypeName;
    std::string VarName;
    json WidgetJson;
    json SlotJson;
    EGeneratedChildRelation Relation = EGeneratedChildRelation::GenericChild;
    FTabRelationData TabData;
};

class FCodeWriter {
public:
    void WriteLine(const std::string& line = std::string())
    {
        Stream_ << std::string(IndentLevel_ * 4, ' ') << line << '\n';
    }

    void Indent()
    {
        ++IndentLevel_;
    }

    void Unindent()
    {
        if (IndentLevel_ > 0) {
            --IndentLevel_;
        }
    }

    std::string ToString() const
    {
        return Stream_.str();
    }

private:
    std::ostringstream Stream_;
    int IndentLevel_ = 0;
};

std::string TrimCopy(const std::string& text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

std::string StripImPrefix(const std::string& typeName)
{
    if (typeName.rfind("Im", 0) == 0 && typeName.size() > 2) {
        return typeName.substr(2);
    }

    return typeName;
}

std::string BuildWidgetTypeNameBase(const std::string& typeName)
{
    const std::string base = StripImPrefix(typeName);
    return base.empty() ? std::string("Widget") : base;
}

bool IsCppIdentifierStartChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalpha(value) != 0 || c == '_';
}

bool IsCppIdentifierContinueChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalnum(value) != 0 || c == '_';
}

const std::unordered_set<std::string>& GetCppKeywords()
{
    static const std::unordered_set<std::string> keywords = {
        "alignas", "alignof", "and", "and_eq", "asm", "auto",
        "bitand", "bitor", "bool", "break", "case", "catch",
        "char", "char8_t", "char16_t", "char32_t", "class", "compl",
        "concept", "const", "consteval", "constexpr", "constinit", "const_cast",
        "continue", "co_await", "co_return", "co_yield", "decltype", "default",
        "delete", "do", "double", "dynamic_cast", "else", "enum",
        "explicit", "export", "extern", "false", "float", "for",
        "friend", "goto", "if", "inline", "int", "long",
        "mutable", "namespace", "new", "noexcept", "not", "not_eq",
        "nullptr", "operator", "or", "or_eq", "private", "protected",
        "public", "register", "reinterpret_cast", "requires", "return", "short",
        "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
        "switch", "template", "this", "thread_local", "throw", "true",
        "try", "typedef", "typeid", "typename", "union", "unsigned",
        "using", "virtual", "void", "volatile", "wchar_t", "while",
        "xor", "xor_eq"
    };

    return keywords;
}

std::string NormalizeIdentifierBase(const std::string& rawName, const std::string& fallbackBase)
{
    const std::string trimmed = TrimCopy(rawName);
    const std::string fallback = TrimCopy(fallbackBase).empty()
        ? std::string("Widget")
        : TrimCopy(fallbackBase);

    auto buildSanitized = [](const std::string& value) {
        std::string result;
        result.reserve(value.size());

        for (char c : value) {
            if (IsCppIdentifierContinueChar(c)) {
                result.push_back(c);
            } else if (result.empty() || result.back() != '_') {
                result.push_back('_');
            }
        }

        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }

        return result;
    };

    std::string sanitizedFallback = buildSanitized(fallback);
    if (sanitizedFallback.empty()) {
        sanitizedFallback = "Widget";
    }
    if (!IsCppIdentifierStartChar(sanitizedFallback.front())) {
        sanitizedFallback.insert(sanitizedFallback.begin(), 'W');
    }
    if (GetCppKeywords().count(sanitizedFallback) > 0) {
        sanitizedFallback.push_back('_');
    }

    std::string sanitized = buildSanitized(trimmed);
    if (sanitized.empty()) {
        return sanitizedFallback;
    }

    if (!IsCppIdentifierStartChar(sanitized.front())) {
        sanitized = sanitizedFallback + sanitized;
    }

    if (GetCppKeywords().count(sanitized) > 0) {
        sanitized.push_back('_');
    }

    return sanitized;
}

bool IsValidNamespaceToken(const std::string& token)
{
    if (token.empty() || !IsCppIdentifierStartChar(token.front())) {
        return false;
    }

    for (char c : token) {
        if (!IsCppIdentifierContinueChar(c)) {
            return false;
        }
    }

    return GetCppKeywords().count(token) == 0;
}

bool IsValidQualifiedName(const std::string& text)
{
    if (text.empty()) {
        return false;
    }

    std::size_t start = 0;
    while (start < text.size()) {
        const std::size_t separator = text.find("::", start);
        const std::string token = text.substr(
            start,
            separator == std::string::npos ? std::string::npos : separator - start);
        if (!IsValidNamespaceToken(token)) {
            return false;
        }

        if (separator == std::string::npos) {
            return true;
        }

        start = separator + 2;
    }

    return true;
}

std::string BuildWidgetHeaderInclude(const std::string& typeName)
{
    const std::string stripped = StripImPrefix(typeName);
    if (!stripped.empty()) {
        return "imwidgetv4/widgets/" + stripped + ".h";
    }

    return "imwidgetv4/widgets/" + typeName + ".h";
}

std::string BuildQualifiedWidgetType(const std::string& typeName)
{
    return "ImWidgetV4::" + typeName;
}

json ExtractWidgetObjectJson(const json& widgetNode)
{
    json widgetJson = json::object();
    widgetJson["Type"] = widgetNode.value("Type", "");
    if (widgetNode.contains("Properties")) {
        widgetJson["Properties"] = widgetNode.at("Properties");
    } else {
        widgetJson["Properties"] = json::object();
    }
    return widgetJson;
}

int FindSerializedIntProperty(const json& widgetJson, const std::string& propertySuffix, int defaultValue)
{
    if (!widgetJson.is_object() || !widgetJson.contains("Properties")) {
        return defaultValue;
    }

    const json& properties = widgetJson.at("Properties");
    if (!properties.is_object()) {
        return defaultValue;
    }

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const std::string key = it.key();
        if (key == propertySuffix ||
            (key.size() > propertySuffix.size() &&
             key.compare(key.size() - propertySuffix.size(), propertySuffix.size(), propertySuffix) == 0)) {
            if (it.value().is_number_integer()) {
                return it.value().get<int>();
            }
        }
    }

    return defaultValue;
}

void CollectGeneratedNodesRecursive(
    const json& widgetNode,
    int parentIndex,
    EGeneratedChildRelation relation,
    const json& slotJson,
    const FTabRelationData* tabData,
    std::vector<FGeneratedNode>& outNodes,
    std::unordered_map<std::string, int>& usedVarNameCounts)
{
    if (!widgetNode.is_object()) {
        return;
    }

    const std::string typeName = widgetNode.value("Type", "");
    if (typeName.empty()) {
        return;
    }

    const json widgetJson = ExtractWidgetObjectJson(widgetNode);
    std::string preferredName;
    if (widgetJson.contains("Properties")) {
        const json& properties = widgetJson.at("Properties");
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            const std::string key = it.key();
            if (key == "ImWidget::Name" || key == "Name" ||
                (key.size() > 4 && key.compare(key.size() - 4, 4, "Name") == 0)) {
                if (it.value().is_string()) {
                    preferredName = it.value().get<std::string>();
                }
                break;
            }
        }
    }

    const std::string normalizedBase =
        NormalizeIdentifierBase(preferredName, BuildWidgetTypeNameBase(typeName));
    int& existingCount = usedVarNameCounts[normalizedBase];
    ++existingCount;

    std::string varName = normalizedBase;
    if (existingCount > 1) {
        varName += std::to_string(existingCount);
    }
    varName += "_";

    FGeneratedNode node;
    node.Index = static_cast<int>(outNodes.size());
    node.ParentIndex = parentIndex;
    node.TypeName = typeName;
    node.VarName = std::move(varName);
    node.WidgetJson = widgetJson;
    node.SlotJson = slotJson;
    node.Relation = relation;
    if (tabData != nullptr) {
        node.TabData = *tabData;
    }

    outNodes.push_back(node);
    const int currentIndex = node.Index;

    if (widgetNode.contains("TabItems") && widgetNode.at("TabItems").is_array()) {
        int tabIndex = 0;
        for (const auto& tabItemJson : widgetNode.at("TabItems")) {
            if (!tabItemJson.is_object()) {
                ++tabIndex;
                continue;
            }

            FTabRelationData currentTabData;
            currentTabData.Index = tabIndex;
            currentTabData.Title = tabItemJson.value("Title", "Tab");
            currentTabData.bEnabled = tabItemJson.value("Enabled", true);
            currentTabData.bClosable = tabItemJson.value("Closable", false);
            currentTabData.bDirty = tabItemJson.value("Dirty", false);

            if (tabItemJson.contains("Content")) {
                CollectGeneratedNodesRecursive(
                    tabItemJson.at("Content"),
                    currentIndex,
                    EGeneratedChildRelation::TabContent,
                    json(),
                    &currentTabData,
                    outNodes,
                    usedVarNameCounts);
            }

            ++tabIndex;
        }
        return;
    }

    if (widgetNode.contains("Content")) {
        CollectGeneratedNodesRecursive(
            widgetNode.at("Content"),
            currentIndex,
            EGeneratedChildRelation::ButtonContent,
            json(),
            nullptr,
            outNodes,
            usedVarNameCounts);
        return;
    }

    if (widgetNode.contains("Header")) {
        CollectGeneratedNodesRecursive(
            widgetNode.at("Header"),
            currentIndex,
            EGeneratedChildRelation::ExpandableHeader,
            json(),
            nullptr,
            outNodes,
            usedVarNameCounts);
    }

    if (widgetNode.contains("Body")) {
        CollectGeneratedNodesRecursive(
            widgetNode.at("Body"),
            currentIndex,
            EGeneratedChildRelation::ExpandableBody,
            json(),
            nullptr,
            outNodes,
            usedVarNameCounts);
    }

    if (widgetNode.contains("Children") && widgetNode.at("Children").is_array()) {
        for (const auto& childJson : widgetNode.at("Children")) {
            const json childSlotJson = childJson.contains("Slot") ? childJson.at("Slot") : json();
            CollectGeneratedNodesRecursive(
                childJson,
                currentIndex,
                typeName == "ImScrollBox"
                    ? EGeneratedChildRelation::ScrollContent
                    : EGeneratedChildRelation::GenericChild,
                childSlotJson,
                nullptr,
                outNodes,
                usedVarNameCounts);
        }
    }
}

std::vector<FGeneratedNode> BuildGeneratedNodes(const std::shared_ptr<ImWidget>& rootWidget)
{
    std::vector<FGeneratedNode> nodes;
    if (!rootWidget) {
        return nodes;
    }

    std::unordered_map<std::string, int> usedVarNameCounts;
    const json rootJson = WidgetSerializer::SerializeWidgetTree(rootWidget);
    CollectGeneratedNodesRecursive(
        rootJson,
        -1,
        EGeneratedChildRelation::GenericChild,
        json(),
        nullptr,
        nodes,
        usedVarNameCounts);
    return nodes;
}

std::vector<std::string> CollectWidgetIncludes(const std::vector<FGeneratedNode>& nodes)
{
    std::vector<std::string> includes;
    includes.reserve(nodes.size() + 2);
    includes.push_back("imwidgetv4/core/Slot.h");

    std::unordered_set<std::string> seen;
    seen.insert(includes.front());
    for (const FGeneratedNode& node : nodes) {
        const std::string includePath = BuildWidgetHeaderInclude(node.TypeName);
        if (seen.insert(includePath).second) {
            includes.push_back(includePath);
        }
    }

    std::sort(includes.begin() + 1, includes.end());
    return includes;
}

void EmitHeader(
    const std::vector<FGeneratedNode>& nodes,
    const FCodeGenOptions& options,
    FGeneratedFilePair& outFiles)
{
    FCodeWriter writer;
    writer.WriteLine("#pragma once");
    writer.WriteLine();
    writer.WriteLine("#include <imwidgetv4/widgets/UserWidget.h>");
    writer.WriteLine("#include <memory>");
    writer.WriteLine();

    std::vector<std::string> forwardTypes;
    std::unordered_set<std::string> seenTypes;
    for (const FGeneratedNode& node : nodes) {
        if (seenTypes.insert(node.TypeName).second) {
            forwardTypes.push_back(node.TypeName);
        }
    }
    std::sort(forwardTypes.begin(), forwardTypes.end());

    writer.WriteLine("namespace ImWidgetV4 {");
    writer.Indent();
    writer.WriteLine("class ImWidget;");
    for (const std::string& typeName : forwardTypes) {
        writer.WriteLine("class " + typeName + ";");
    }
    writer.Unindent();
    writer.WriteLine("} // namespace ImWidgetV4");
    writer.WriteLine();

    if (!options.Namespace.empty()) {
        writer.WriteLine("namespace " + options.Namespace + " {");
        writer.WriteLine();
    }

    writer.WriteLine("class " + options.ClassName + " : public " + options.BaseClass + " {");
    writer.WriteLine("public:");
    writer.Indent();
    writer.WriteLine(options.ClassName + "();");
    writer.Unindent();
    writer.WriteLine();
    writer.WriteLine("protected:");
    writer.Indent();
    writer.WriteLine("std::shared_ptr<ImWidgetV4::ImWidget> RebuildWidget() override;");
    writer.Unindent();
    writer.WriteLine();
    writer.WriteLine("private:");
    writer.Indent();
    writer.WriteLine("//===Auto Gen Begin=== (Members)");
    for (const FGeneratedNode& node : nodes) {
        writer.WriteLine(
            "std::shared_ptr<" + BuildQualifiedWidgetType(node.TypeName) + "> " + node.VarName + ";");
    }
    writer.WriteLine("//===Auto Gen End=== (Members)");
    writer.Unindent();
    writer.WriteLine("};");

    if (!options.Namespace.empty()) {
        writer.WriteLine();
        writer.WriteLine("} // namespace " + options.Namespace);
    }

    outFiles.HeaderText = writer.ToString();
}

std::string BuildJsonLiteral(const json& value)
{
    return "R\"IMWJSON(" + value.dump() + ")IMWJSON\"";
}

std::string BuildStringLiteral(const std::string& value)
{
    std::string result = "\"";
    result.reserve(value.size() + 8);

    for (char c : value) {
        switch (c) {
        case '\\':
            result += "\\\\";
            break;
        case '\"':
            result += "\\\"";
            break;
        case '\n':
            result += "\\n";
            break;
        case '\r':
            result += "\\r";
            break;
        case '\t':
            result += "\\t";
            break;
        default:
            result.push_back(c);
            break;
        }
    }

    result.push_back('\"');
    return result;
}

void EmitWidgetCreationCode(FCodeWriter& writer, const FGeneratedNode& node)
{
    writer.WriteLine(
        node.VarName + " = std::make_shared<" + BuildQualifiedWidgetType(node.TypeName) + ">();");
    writer.WriteLine(
        node.VarName + "->FromJson(ParseGeneratedJson(" + BuildJsonLiteral(node.WidgetJson) + "));");
}

void EmitRelationCode(FCodeWriter& writer, const FGeneratedNode& parent, const FGeneratedNode& child)
{
    switch (child.Relation) {
    case EGeneratedChildRelation::GenericChild:
        writer.WriteLine(parent.VarName + "->AddChild(" + child.VarName + ");");
        if (child.SlotJson.is_object()) {
            writer.WriteLine("if (auto* slot = " + parent.VarName + "->GetSlotForChild(" + child.VarName + ")) {");
            writer.Indent();
            writer.WriteLine(
                "slot->FromJson(ParseGeneratedJson(" + BuildJsonLiteral(child.SlotJson) + "));");
            writer.Unindent();
            writer.WriteLine("}");
        }
        break;

    case EGeneratedChildRelation::ButtonContent:
        writer.WriteLine(parent.VarName + "->SetContent(" + child.VarName + ");");
        break;

    case EGeneratedChildRelation::ScrollContent:
        writer.WriteLine(parent.VarName + "->SetContent(" + child.VarName + ");");
        break;

    case EGeneratedChildRelation::ExpandableHeader:
        writer.WriteLine(parent.VarName + "->SetHeader(" + child.VarName + ");");
        break;

    case EGeneratedChildRelation::ExpandableBody:
        writer.WriteLine(parent.VarName + "->SetBody(" + child.VarName + ");");
        break;

    case EGeneratedChildRelation::TabContent: {
        std::ostringstream line;
        line << "const int " << child.VarName << "TabIndex = " << parent.VarName << "->AddTab("
             << BuildStringLiteral(child.TabData.Title)
             << ", " << child.VarName << ");";
        writer.WriteLine(line.str());
        writer.WriteLine(
            parent.VarName + "->SetTabEnabled(" + child.VarName + "TabIndex, " +
            std::string(child.TabData.bEnabled ? "true" : "false") + ");");
        writer.WriteLine(
            parent.VarName + "->SetTabClosable(" + child.VarName + "TabIndex, " +
            std::string(child.TabData.bClosable ? "true" : "false") + ");");
        writer.WriteLine(
            parent.VarName + "->SetTabDirty(" + child.VarName + "TabIndex, " +
            std::string(child.TabData.bDirty ? "true" : "false") + ");");
        break;
    }
    }
}

void EmitSource(
    const std::vector<FGeneratedNode>& nodes,
    const FCodeGenOptions& options,
    FGeneratedFilePair& outFiles)
{
    FCodeWriter writer;
    writer.WriteLine("#include \"" + outFiles.HeaderFileName + "\"");
    for (const std::string& includePath : CollectWidgetIncludes(nodes)) {
        writer.WriteLine("#include <" + includePath + ">");
    }
    writer.WriteLine("#include <nlohmann/json.hpp>");
    writer.WriteLine();
    writer.WriteLine("namespace {");
    writer.Indent();
    writer.WriteLine("using json = nlohmann::ordered_json;");
    writer.WriteLine();
    writer.WriteLine("json ParseGeneratedJson(const char* text)");
    writer.WriteLine("{");
    writer.Indent();
    writer.WriteLine("return json::parse(text);");
    writer.Unindent();
    writer.WriteLine("}");
    writer.Unindent();
    writer.WriteLine("} // namespace");
    writer.WriteLine();

    if (!options.Namespace.empty()) {
        writer.WriteLine("namespace " + options.Namespace + " {");
        writer.WriteLine();
    }

    writer.WriteLine(options.ClassName + "::" + options.ClassName + "()");
    writer.WriteLine("{");
    writer.Indent();
    writer.WriteLine("SetName(\"" + options.ClassName + "\");");
    writer.Unindent();
    writer.WriteLine("}");
    writer.WriteLine();

    writer.WriteLine("std::shared_ptr<ImWidgetV4::ImWidget> " + options.ClassName + "::RebuildWidget()");
    writer.WriteLine("{");
    writer.Indent();
    writer.WriteLine("//===Auto Gen Begin=== (RebuildWidget)");

    for (const FGeneratedNode& node : nodes) {
        EmitWidgetCreationCode(writer, node);
    }

    if (!nodes.empty()) {
        writer.WriteLine();
    }

    for (const FGeneratedNode& node : nodes) {
        if (node.ParentIndex < 0) {
            continue;
        }

        const FGeneratedNode& parent = nodes[static_cast<std::size_t>(node.ParentIndex)];
        EmitRelationCode(writer, parent, node);
    }

    for (const FGeneratedNode& node : nodes) {
        if (node.TypeName != "ImTabView") {
            continue;
        }

        const int activeTabIndex = FindSerializedIntProperty(node.WidgetJson, "::ActiveTabIndex", -1);
        if (activeTabIndex >= 0) {
            writer.WriteLine(node.VarName + "->SetActiveTab(" + std::to_string(activeTabIndex) + ");");
        }
    }

    writer.WriteLine();
    if (!nodes.empty()) {
        writer.WriteLine("return " + nodes.front().VarName + ";");
    } else {
        writer.WriteLine("return nullptr;");
    }
    writer.WriteLine("//===Auto Gen End=== (RebuildWidget)");
    writer.Unindent();
    writer.WriteLine("}");

    if (!options.Namespace.empty()) {
        writer.WriteLine();
        writer.WriteLine("} // namespace " + options.Namespace);
    }

    outFiles.SourceText = writer.ToString();
}

FCodeGenResult BuildInvalidResult(const std::string& errorMessage)
{
    FCodeGenResult result;
    result.bSuccess = false;
    result.ErrorMessage = errorMessage;
    return result;
}

} // namespace

FCodeGenResult WidgetTreeToCppGenerator::Generate(
    const std::shared_ptr<ImWidget>& rootWidget,
    const FCodeGenOptions& options)
{
    if (!rootWidget) {
        return BuildInvalidResult("Cannot generate code for an empty widget tree.");
    }

    if (!IsValidNamespaceToken(options.ClassName)) {
        return BuildInvalidResult("Code generation requires a valid C++ class name.");
    }

    if (!options.Namespace.empty()) {
        std::size_t start = 0;
        while (start < options.Namespace.size()) {
            const std::size_t separator = options.Namespace.find("::", start);
            const std::string token = options.Namespace.substr(
                start,
                separator == std::string::npos ? std::string::npos : separator - start);
            if (!IsValidNamespaceToken(token)) {
                return BuildInvalidResult("Code generation requires a valid C++ namespace.");
            }

            if (separator == std::string::npos) {
                break;
            }

            start = separator + 2;
        }
    }

    FCodeGenResult result;
    result.bSuccess = true;
    result.Files.HeaderFileName = options.ClassName + ".h";
    result.Files.SourceFileName = options.ClassName + ".cpp";

    const std::vector<FGeneratedNode> nodes = BuildGeneratedNodes(rootWidget);
    if (nodes.empty()) {
        return BuildInvalidResult("Serialized widget tree did not contain a valid root widget.");
    }

    EmitHeader(nodes, options, result.Files);
    EmitSource(nodes, options, result.Files);
    return result;
}

FCodeGenResult WidgetTreeToCppGenerator::Generate(
    const EditorDocument& document,
    const FCodeGenOptions& options)
{
    return Generate(document.GetRootWidget(), options);
}

} // namespace ImWidgetV4Editor
