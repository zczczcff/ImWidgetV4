#include "ReflectionDetailsView.h"
#include "../editor/EditorTheme.h"
#include "../editor/EditorLocalization.h"
#include "PropertyEditorWidgets.h"

#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor::PropertyEditorWidgets;
namespace Reflection = ImWidgetV4::Reflection;

namespace {

std::shared_ptr<ImTextBlock> MakeText(
    const std::string& text,
    float fontSize,
    const FColor& color)
{
    auto widget = std::make_shared<ImTextBlock>();
    widget->SetText(text);
    widget->SetFontSize(fontSize);
    widget->SetTextColor(color);
    return widget;
}

std::string FormatFloat(float value)
{
    std::ostringstream stream;
    stream.setf(std::ios::fixed);
    stream.precision(2);
    stream << value;
    return stream.str();
}

std::string JsonValueToString(const nlohmann::ordered_json& value)
{
    if (value.is_string()) {
        return value.get<std::string>();
    }

    if (value.is_boolean()) {
        return value.get<bool>() ? "true" : "false";
    }

    if (value.is_number_integer()) {
        return std::to_string(value.get<int>());
    }

    if (value.is_number_float()) {
        return FormatFloat(value.get<float>());
    }

    if (value.is_array()) {
        if (value.size() == 2 && value[0].is_number() && value[1].is_number()) {
            return "(" + JsonValueToString(value[0]) + ", " + JsonValueToString(value[1]) + ")";
        }

        if (value.size() == 4 &&
            value[0].is_number_integer() &&
            value[1].is_number_integer() &&
            value[2].is_number_integer() &&
            value[3].is_number_integer()) {
            return "rgba(" +
                std::to_string(value[0].get<int>()) + ", " +
                std::to_string(value[1].get<int>()) + ", " +
                std::to_string(value[2].get<int>()) + ", " +
                std::to_string(value[3].get<int>()) + ")";
        }

        return std::to_string(value.size()) + " items";
    }

    if (value.is_object()) {
        return "{...}";
    }

    if (value.is_null()) {
        return "null";
    }

    return value.dump();
}

std::string TrimCopy(const std::string& text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return "";
    }

    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::vector<std::string> SplitCommaSeparated(const std::string& text)
{
    std::vector<std::string> tokens;
    std::stringstream stream(text);
    std::string item;

    while (std::getline(stream, item, ',')) {
        tokens.push_back(TrimCopy(item));
    }

    return tokens;
}

bool TryParseInt(const std::string& text, int& outValue)
{
    try {
        const std::string trimmed = TrimCopy(text);
        std::size_t consumed = 0;
        const int value = std::stoi(trimmed, &consumed);
        if (consumed != trimmed.size()) {
            return false;
        }
        outValue = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool TryParseFloat(const std::string& text, float& outValue)
{
    try {
        const std::string trimmed = TrimCopy(text);
        std::size_t consumed = 0;
        const float value = std::stof(trimmed, &consumed);
        if (consumed != trimmed.size()) {
            return false;
        }
        outValue = value;
        return true;
    } catch (...) {
        return false;
    }
}

bool TryParseColor(const std::string& text, FColor& outColor)
{
    const std::vector<std::string> tokens = SplitCommaSeparated(text);
    if (tokens.size() != 4) {
        return false;
    }

    int rgba[4] = {};
    for (int index = 0; index < 4; ++index) {
        if (!TryParseInt(tokens[index], rgba[index])) {
            return false;
        }
        rgba[index] = std::clamp(rgba[index], 0, 255);
    }

    outColor = FColor::FromBytes(rgba[0], rgba[1], rgba[2], rgba[3]);
    return true;
}

bool TryParseVec2(const std::string& text, FVector2& outValue)
{
    const std::vector<std::string> tokens = SplitCommaSeparated(text);
    if (tokens.size() != 2) {
        return false;
    }

    float values[2] = {};
    for (int index = 0; index < 2; ++index) {
        if (!TryParseFloat(tokens[index], values[index])) {
            return false;
        }
    }

    outValue = FVector2(values[0], values[1]);
    return true;
}

std::vector<std::string> JsonValueToStringArray(const nlohmann::ordered_json& value)
{
    if (!value.is_array()) {
        return {};
    }

    std::vector<std::string> items;
    items.reserve(value.size());
    for (const auto& item : value) {
        items.push_back(JsonValueToString(item));
    }
    return items;
}

std::string JoinLines(const std::vector<std::string>& lines)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < lines.size(); ++index) {
        if (index > 0) {
            stream << "\n";
        }
        stream << lines[index];
    }
    return stream.str();
}

bool TryParseStringArrayLines(const std::string& text, std::vector<std::string>& outItems)
{
    outItems.clear();
    std::stringstream stream(text);
    std::string line;
    while (std::getline(stream, line)) {
        outItems.push_back(TrimCopy(line));
    }
    return true;
}

std::string FormatColor(const FColor& color)
{
    const int r = static_cast<int>(std::round(color.R * 255.0f));
    const int g = static_cast<int>(std::round(color.G * 255.0f));
    const int b = static_cast<int>(std::round(color.B * 255.0f));
    const int a = static_cast<int>(std::round(color.A * 255.0f));
    return std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a);
}

std::array<int, 4> GetColorChannels(const FColor& color)
{
    return {
        static_cast<int>(std::round(color.R * 255.0f)),
        static_cast<int>(std::round(color.G * 255.0f)),
        static_cast<int>(std::round(color.B * 255.0f)),
        static_cast<int>(std::round(color.A * 255.0f))
    };
}

FColor JsonColorToFColor(const nlohmann::ordered_json& value, const FColor& fallback = FColor::White)
{
    if (!value.is_array() || value.size() != 4) {
        return fallback;
    }

    return FColor::FromBytes(
        value[0].get<int>(),
        value[1].get<int>(),
        value[2].get<int>(),
        value[3].get<int>());
}

FVector2 JsonVec2ToFVector2(const nlohmann::ordered_json& value, const FVector2& fallback = FVector2::Zero)
{
    if (!value.is_array() || value.size() != 2) {
        return fallback;
    }

    return FVector2(value[0].get<float>(), value[1].get<float>());
}

std::string FormatVec2(const FVector2& value)
{
    return FormatFloat(value.X) + ", " + FormatFloat(value.Y);
}

std::shared_ptr<ImEditableText> CreateInspectorTextEditor(
    const std::string& initialText,
    const std::string& hintText = {})
{
    auto editor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*editor, false);
    editor->SetText(initialText);
    if (!hintText.empty()) {
        editor->SetHintText(hintText);
    }
    return editor;
}

std::shared_ptr<ImWidget> BuildCompactNumberEditors(
    const std::vector<std::pair<std::string, std::shared_ptr<ImEditableText>>>& labeledEditors)
{
    std::vector<std::pair<std::string, std::shared_ptr<ImWidget>>> widgets;
    widgets.reserve(labeledEditors.size());
    for (const auto& entry : labeledEditors) {
        widgets.push_back({entry.first, entry.second});
    }
    return MakeInspectorCompactLabeledEditors(widgets);
}

std::shared_ptr<ImWidget> BuildWidgetMetadataRows(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return nullptr;
    }

    auto rows = std::make_shared<ImVerticalBox>();
    rows->SetSpacing(6.0f);

    const FGeometry geometry = widget->GetGeometry();
    std::string parentLabel = EditorText("Details.RootParent", "<root>").Resolve();
    if (auto parent = widget->GetParent()) {
        parentLabel = parent->GetTypeName();
        if (!parent->GetName().empty()) {
            parentLabel += " [" + parent->GetName() + "]";
        }
    }

    rows->AddChild(MakeInspectorPropertyRow(EditorText("Details.Type", "Type").Resolve(), MakeInspectorReadOnlyField(widget->GetTypeName())));
    rows->AddChild(MakeInspectorPropertyRow(
        EditorText("Details.Name", "Name").Resolve(),
        MakeInspectorReadOnlyField(widget->GetName().empty() ? EditorText("Details.Unnamed", "<unnamed>").Resolve() : widget->GetName())));
    rows->AddChild(MakeInspectorPropertyRow(EditorText("Details.Position", "Position").Resolve(), MakeInspectorReadOnlyField(FormatVec2(geometry.Position))));
    rows->AddChild(MakeInspectorPropertyRow(EditorText("Details.Size", "Size").Resolve(), MakeInspectorReadOnlyField(FormatVec2(geometry.Size))));
    rows->AddChild(MakeInspectorPropertyRow(EditorText("Details.Parent", "Parent").Resolve(), MakeInspectorReadOnlyField(parentLabel)));
    return rows;
}

std::shared_ptr<ImWidget> MakeSectionLabelWidget(const std::string& title)
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(title);
    label->SetWrapText(false);
    label->SetFontSize(14.0f);
    label->SetTextColor(GetEditorPanelTitleColor());
    return label;
}

} // namespace

ReflectionDetailsView::ReflectionDetailsView()
    : ImUserWidget()
{
    SetHitTestVisible(true);
}

void ReflectionDetailsView::SetTargets(
    const std::shared_ptr<ReflectableObject>& target,
    const std::shared_ptr<ImSlot>& slotTarget)
{
    if (m_Target == target && m_SlotTarget == slotTarget) {
        return;
    }

    CaptureCurrentViewState();
    m_Target = target;
    m_SlotTarget = slotTarget;
    Rebuild();
    RestoreCurrentViewState();
}

void ReflectionDetailsView::SetTarget(const std::shared_ptr<ReflectableObject>& target)
{
    SetTargets(target, m_SlotTarget);
}

void ReflectionDetailsView::SetSlotTarget(const std::shared_ptr<ImSlot>& slotTarget)
{
    SetTargets(m_Target, slotTarget);
}

void ReflectionDetailsView::RebuildPreservingViewState()
{
    CaptureCurrentViewState();
    Rebuild();
    RestoreCurrentViewState();
}

bool ReflectionDetailsView::SetSectionExpanded(const std::string& path, bool expanded)
{
    auto outlineView = GetCurrentOutlineView();
    if (!outlineView) {
        return false;
    }

    const auto itemIt = m_CurrentPathItems.find(path);
    if (itemIt == m_CurrentPathItems.end() || itemIt->second == nullptr) {
        return false;
    }

    outlineView->SetItemExpanded(itemIt->second, expanded, false);
    if (!m_CurrentStateKey.empty()) {
        m_ViewStatesByKey[m_CurrentStateKey].ExpandedByPath[path] = expanded;
    }
    return true;
}

bool ReflectionDetailsView::IsSectionExpanded(const std::string& path) const
{
    const auto itemIt = m_CurrentPathItems.find(path);
    return itemIt != m_CurrentPathItems.end() &&
        itemIt->second != nullptr &&
        itemIt->second->Expanded;
}

ImWidget::Ptr ReflectionDetailsView::RebuildWidget()
{
    m_CurrentItemPaths.clear();
    m_CurrentPathItems.clear();
    m_CurrentStateKey = BuildCurrentStateKey();

    if (!m_Target && !m_SlotTarget) {
        return BuildEmptyState();
    }

    auto outlineView = std::make_shared<ImOutlineView>();
    outlineView->SetSupportsKeyboardFocus(true);
    outlineView->SetStyle(MakeEditorInspectorOutlineStyle(outlineView->GetStyle()));

    if (m_Target) {
        if (auto widget = std::dynamic_pointer_cast<ImWidget>(m_Target)) {
            BuildCommonSection(*outlineView, widget);
            BuildObjectSection(*outlineView, m_Target, EditorText("Details.Properties", "Properties").Resolve());
        } else {
            BuildObjectSection(*outlineView, m_Target, m_Target->GetTypeName());
        }
    }
    if (m_SlotTarget) {
        BuildObjectSection(*outlineView, m_SlotTarget, EditorText("Details.Slot", "Slot").Resolve());
    }

    return outlineView;
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildEmptyState() const
{
    auto outlineView = std::make_shared<ImOutlineView>();
    outlineView->SetStyle(MakeEditorInspectorOutlineStyle(outlineView->GetStyle()));

    ImOutlineItem* rootItem = outlineView->AddRootItem(MakeSectionLabelWidget(EditorText("Details.Title", "Details").Resolve()));
    if (rootItem) {
        rootItem->Expanded = true;
        outlineView->AddChildItem(
            rootItem,
            MakeText(
                EditorText("Details.EmptyHint", "Select a widget in the designer surface to inspect its reflected properties.").Resolve(),
                12.0f,
                GetEditorPanelBodyColor()));
    }
    return outlineView;
}

void ReflectionDetailsView::BuildCommonSection(
    ImOutlineView& outlineView,
    const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return;
    }

    ImOutlineItem* sectionItem = AddTrackedGroupItem(
        outlineView,
        nullptr,
        "Common",
        EditorText("Details.Common", "Common").Resolve(),
        true);
    if (!sectionItem) {
        return;
    }

    const FGeometry geometry = widget->GetGeometry();
    std::string parentLabel = EditorText("Details.RootParent", "<root>").Resolve();
    if (auto parent = widget->GetParent()) {
        parentLabel = parent->GetTypeName();
        if (!parent->GetName().empty()) {
            parentLabel += " [" + parent->GetName() + "]";
        }
    }

    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow(EditorText("Details.Type", "Type").Resolve(), MakeInspectorReadOnlyField(widget->GetTypeName())));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow(EditorText("Details.Position", "Position").Resolve(), MakeInspectorReadOnlyField(FormatVec2(geometry.Position))));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow(EditorText("Details.Size", "Size").Resolve(), MakeInspectorReadOnlyField(FormatVec2(geometry.Size))));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow(EditorText("Details.Parent", "Parent").Resolve(), MakeInspectorReadOnlyField(parentLabel)));
}

void ReflectionDetailsView::BuildObjectSection(
    ImOutlineView& outlineView,
    const std::shared_ptr<ReflectableObject>& object,
    const std::string& title)
{
    if (!object) {
        return;
    }

    ImOutlineItem* sectionItem = AddTrackedGroupItem(outlineView, nullptr, title, title, true);
    if (!sectionItem) {
        return;
    }

    BuildPropertyItems(outlineView, sectionItem, object, title);
}

void ReflectionDetailsView::BuildPropertyItems(
    ImOutlineView& outlineView,
    ImOutlineItem* parentItem,
    const std::shared_ptr<ReflectableObject>& object,
    const std::string& parentPath)
{
    if (!parentItem || !object) {
        return;
    }

    const auto objectJson = object->ToJson();
    const auto& propertyJson = objectJson.contains("Properties")
        ? objectJson.at("Properties")
        : nlohmann::ordered_json::object();
    const auto properties = Reflection::CollectProperties(object->GetTypeDesc());
    for (const Reflection::FPropertyDesc* property : properties) {
        if (!property) {
            continue;
        }

        auto nestedObject = ResolveNestedObject(object, *property);
        if (nestedObject) {
            const std::string propertyKey = std::string(property->OwnerTypeName) + "::" + property->Name;
            const auto propertyValueIt = propertyJson.find(propertyKey);
            const nlohmann::ordered_json propertyValueJson =
                propertyValueIt != propertyJson.end()
                ? propertyValueIt.value()
                : nlohmann::ordered_json::object();

            if (auto specializedRow = BuildStructPropertyEditorRow(
                    object,
                    *property,
                    property->OwnerTypeName,
                    property->Name,
                    propertyValueJson)) {
                outlineView.AddChildItem(parentItem, specializedRow);
            } else {
                const std::string groupPath = parentPath.empty()
                    ? property->Name
                    : parentPath + "/" + property->Name;
                ImOutlineItem* groupItem = AddTrackedGroupItem(
                    outlineView,
                    parentItem,
                    groupPath,
                    property->Name,
                    false);
                if (groupItem) {
                    BuildPropertyItems(outlineView, groupItem, nestedObject, groupPath);
                }
            }
            continue;
        }

        outlineView.AddChildItem(
            parentItem,
            BuildPropertyEditorRow(
                object,
                *property,
                propertyJson));
    }
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildPropertyEditorRow(
    const std::shared_ptr<ReflectableObject>& owner,
    const Reflection::FPropertyDesc& property,
    const nlohmann::ordered_json& objectJson) const
{
    const std::string propertyName = property.Name;
    const std::string propertyClassName = property.OwnerTypeName;
    const std::string propertyKey = propertyClassName + "::" + propertyName;
    const std::string labelText = propertyName;
    const auto propertyValueIt = objectJson.find(propertyKey);
    const nlohmann::ordered_json currentJsonValue =
        propertyValueIt != objectJson.end()
        ? propertyValueIt.value()
        : nlohmann::ordered_json();
    const auto applyJsonValue = [this, owner, propertyName, propertyClassName](const nlohmann::ordered_json& value) {
        if (!owner) {
            return false;
        }

        ReflectionDetailsView* mutableThis = const_cast<ReflectionDetailsView*>(this);
        if (mutableThis->OnPropertyValueCommitted.IsBound()) {
            mutableThis->OnPropertyValueCommitted.Broadcast(
                *mutableThis,
                owner,
                propertyClassName,
                propertyName,
                value);
            return true;
        }

        auto serialized = owner->ToJson();
        serialized["Properties"][propertyClassName + "::" + propertyName] = value;
        owner->FromJson(serialized);
        mutableThis->RebuildPreservingViewState();
        mutableThis->OnPropertiesChanged.Broadcast(*mutableThis);
        return true;
    };

    if (property.Kind == Reflection::EPropertyKind::Bool) {
        auto toggle = std::make_shared<ImSwitch>();
        ApplyInspectorSwitchStyle(*toggle);
        toggle->SetChecked(currentJsonValue.is_boolean() ? currentJsonValue.get<bool>() : false);
        toggle->OnCheckStateChanged.AddLambda(
            [applyJsonValue](ImSwitch&, bool checked) {
                applyJsonValue(checked);
            });
        return MakeInspectorRightAlignedPropertyRow(labelText, toggle);
    }

    if (property.Kind == Reflection::EPropertyKind::String) {
        auto editor = CreateInspectorTextEditor(currentJsonValue.is_string() ? currentJsonValue.get<std::string>() : std::string());
        editor->OnTextCommitted.AddLambda(
            [applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                applyJsonValue(text);
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(text);
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.Kind == Reflection::EPropertyKind::Int) {
        auto editor = CreateInspectorTextEditor(currentJsonValue.is_number_integer() ? std::to_string(currentJsonValue.get<int>()) : "0", "Integer");
        editor->OnTextCommitted.AddLambda(
            [applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                int value = 0;
                if (TryParseInt(text, value)) {
                    applyJsonValue(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(std::to_string(value));
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.Kind == Reflection::EPropertyKind::Float) {
        auto editor = CreateInspectorTextEditor(currentJsonValue.is_number() ? FormatFloat(currentJsonValue.get<float>()) : FormatFloat(0.0f), "Float");
        editor->OnTextCommitted.AddLambda(
            [applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                float value = 0.0f;
                if (TryParseFloat(text, value)) {
                    applyJsonValue(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(FormatFloat(value));
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.Kind == Reflection::EPropertyKind::Enum) {
        auto editor = std::make_shared<ImComboBox>();
        ApplyInspectorComboBoxStyle(*editor);
        FReflectedOptionalProperty optionalProperty(owner.get(), &property);
        const auto& options = optionalProperty.GetOptionList();
        editor->SetItems(options);

        const std::string currentOption =
            currentJsonValue.is_string()
            ? currentJsonValue.get<std::string>()
            : optionalProperty.GetOptionString();
        for (int index = 0; index < static_cast<int>(options.size()); ++index) {
            if (options[index] == currentOption) {
                editor->SetSelectedIndex(index);
                break;
            }
        }

        editor->OnSelectionChanged.AddLambda(
            [this, owner, propertyName, propertyClassName](ImComboBox&, int index) {
                if (!owner || index < 0) {
                    return;
                }

                const std::string key = propertyClassName + "::" + propertyName;
                nlohmann::ordered_json beforeSerialized = owner->ToJson();
                if (!beforeSerialized.contains("Properties") || !beforeSerialized["Properties"].contains(key)) {
                    return;
                }

                const Reflection::FPropertyDesc* reloaded = Reflection::FindProperty(owner->GetTypeDesc(), propertyName, propertyClassName);
                if (!reloaded) {
                    return;
                }

                FReflectedOptionalProperty optional(owner.get(), reloaded);
                optional.SetOptionByIndex(index);

                nlohmann::ordered_json afterSerialized = owner->ToJson();
                const nlohmann::ordered_json afterValue =
                    afterSerialized.contains("Properties") && afterSerialized["Properties"].contains(key)
                    ? afterSerialized["Properties"].at(key)
                    : nlohmann::ordered_json();

                ReflectionDetailsView* mutableThis = const_cast<ReflectionDetailsView*>(this);
                if (mutableThis->OnPropertyValueCommitted.IsBound()) {
                    owner->FromJson(beforeSerialized);
                    mutableThis->OnPropertyValueCommitted.Broadcast(
                        *mutableThis,
                        owner,
                        propertyClassName,
                        propertyName,
                        afterValue);
                    return;
                }

                mutableThis->RebuildPreservingViewState();
                mutableThis->OnPropertiesChanged.Broadcast(*mutableThis);
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.Kind == Reflection::EPropertyKind::Color) {
        FColor currentColor = JsonColorToFColor(currentJsonValue);

        auto picker = std::make_shared<ImColorPicker>();
        picker->SetColor(currentColor);

        auto rEditor = CreateInspectorTextEditor(std::to_string(GetColorChannels(currentColor)[0]), "R");
        auto gEditor = CreateInspectorTextEditor(std::to_string(GetColorChannels(currentColor)[1]), "G");
        auto bEditor = CreateInspectorTextEditor(std::to_string(GetColorChannels(currentColor)[2]), "B");
        auto aEditor = CreateInspectorTextEditor(std::to_string(GetColorChannels(currentColor)[3]), "A");

        const auto syncEditorsFromJson =
            [owner, propertyName, propertyClassName, weakPicker = std::weak_ptr<ImColorPicker>(picker), weakR = std::weak_ptr<ImEditableText>(rEditor), weakG = std::weak_ptr<ImEditableText>(gEditor), weakB = std::weak_ptr<ImEditableText>(bEditor), weakA = std::weak_ptr<ImEditableText>(aEditor)]() {
                const auto refreshed = owner ? owner->ToJson() : nlohmann::ordered_json::object();
                const auto& refreshedProperties = refreshed.contains("Properties")
                    ? refreshed.at("Properties")
                    : nlohmann::ordered_json::object();
                const auto key = propertyClassName + "::" + propertyName;
                if (!refreshedProperties.contains(key)) {
                    return;
                }

                FColor refreshedColor;
                if (!TryParseColor(JsonValueToString(refreshedProperties.at(key)), refreshedColor)) {
                    return;
                }

                const auto channels = GetColorChannels(refreshedColor);
                if (auto lockedPicker = weakPicker.lock()) {
                    lockedPicker->SetColor(refreshedColor);
                }
                if (auto locked = weakR.lock()) {
                    locked->SetText(std::to_string(channels[0]));
                }
                if (auto locked = weakG.lock()) {
                    locked->SetText(std::to_string(channels[1]));
                }
                if (auto locked = weakB.lock()) {
                    locked->SetText(std::to_string(channels[2]));
                }
                if (auto locked = weakA.lock()) {
                    locked->SetText(std::to_string(channels[3]));
                }
            };

        picker->OnColorCommitted.AddLambda(
            [applyJsonValue, syncEditorsFromJson](ImColorPicker&, const FColor& color) {
                const auto channels = GetColorChannels(color);
                applyJsonValue(nlohmann::ordered_json::array({channels[0], channels[1], channels[2], channels[3]}));
                syncEditorsFromJson();
            });

        const auto commitChannelEditors =
            [applyJsonValue, syncEditorsFromJson, weakR = std::weak_ptr<ImEditableText>(rEditor), weakG = std::weak_ptr<ImEditableText>(gEditor), weakB = std::weak_ptr<ImEditableText>(bEditor), weakA = std::weak_ptr<ImEditableText>(aEditor)]() {
                auto lockedR = weakR.lock();
                auto lockedG = weakG.lock();
                auto lockedB = weakB.lock();
                auto lockedA = weakA.lock();
                if (!lockedR || !lockedG || !lockedB || !lockedA) {
                    return;
                }

                int r = 0;
                int g = 0;
                int b = 0;
                int a = 0;
                if (!TryParseInt(lockedR->GetText(), r) ||
                    !TryParseInt(lockedG->GetText(), g) ||
                    !TryParseInt(lockedB->GetText(), b) ||
                    !TryParseInt(lockedA->GetText(), a)) {
                    syncEditorsFromJson();
                    return;
                }

                applyJsonValue(nlohmann::ordered_json::array({
                    std::clamp(r, 0, 255),
                    std::clamp(g, 0, 255),
                    std::clamp(b, 0, 255),
                    std::clamp(a, 0, 255)}));
                syncEditorsFromJson();
            };

        rEditor->OnTextCommitted.AddLambda([commitChannelEditors](ImEditableText&, const std::string&) { commitChannelEditors(); });
        gEditor->OnTextCommitted.AddLambda([commitChannelEditors](ImEditableText&, const std::string&) { commitChannelEditors(); });
        bEditor->OnTextCommitted.AddLambda([commitChannelEditors](ImEditableText&, const std::string&) { commitChannelEditors(); });
        aEditor->OnTextCommitted.AddLambda([commitChannelEditors](ImEditableText&, const std::string&) { commitChannelEditors(); });

        auto group = std::make_shared<ImVerticalBox>();
        group->SetSpacing(6.0f);
        group->AddChild(picker);
        group->AddChild(MakeInspectorCompactLabeledEditors({
            {"R", rEditor},
            {"G", gEditor},
            {"B", bEditor},
            {"A", aEditor}}));
        return MakeInspectorVerticalPropertyRow(labelText, group);
    }

    if (property.Kind == Reflection::EPropertyKind::Vec2) {
        FVector2 currentValue = JsonVec2ToFVector2(currentJsonValue);

        auto xEditor = CreateInspectorTextEditor(FormatFloat(currentValue.X), "X");
        auto yEditor = CreateInspectorTextEditor(FormatFloat(currentValue.Y), "Y");
        const auto commitVec2 = [owner, propertyName, propertyClassName, applyJsonValue, weakX = std::weak_ptr<ImEditableText>(xEditor), weakY = std::weak_ptr<ImEditableText>(yEditor)]() {
            auto lockedX = weakX.lock();
            auto lockedY = weakY.lock();
            if (!lockedX || !lockedY) {
                return;
            }

            float x = 0.0f;
            float y = 0.0f;
            if (TryParseFloat(lockedX->GetText(), x) && TryParseFloat(lockedY->GetText(), y)) {
                applyJsonValue(nlohmann::ordered_json::array({x, y}));
            }

            const auto refreshed = owner ? owner->ToJson() : nlohmann::ordered_json::object();
            const auto& refreshedProperties = refreshed.contains("Properties")
                ? refreshed.at("Properties")
                : nlohmann::ordered_json::object();
            const auto propertyKey = propertyClassName + "::" + propertyName;
            if (refreshedProperties.contains(propertyKey) &&
                refreshedProperties.at(propertyKey).is_array() &&
                refreshedProperties.at(propertyKey).size() == 2) {
                lockedX->SetText(FormatFloat(refreshedProperties.at(propertyKey)[0].get<float>()));
                lockedY->SetText(FormatFloat(refreshedProperties.at(propertyKey)[1].get<float>()));
            }
        };

        xEditor->OnTextCommitted.AddLambda(
            [commitVec2](ImEditableText&, const std::string&) {
                commitVec2();
            });
        yEditor->OnTextCommitted.AddLambda(
            [commitVec2](ImEditableText&, const std::string&) {
                commitVec2();
            });
        return MakeInspectorPropertyRow(
            labelText,
            BuildCompactNumberEditors({
                {"X", xEditor},
                {"Y", yEditor}}));
    }

    if (property.Kind == Reflection::EPropertyKind::StringArray) {
        std::vector<std::string> items;
        if (currentJsonValue.is_array()) {
            items = JsonValueToStringArray(currentJsonValue);
        }

        auto editor = CreateInspectorTextEditor(JoinLines(items), "One item per line");
        editor->OnTextCommitted.AddLambda(
            [owner, propertyName, propertyClassName, applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                std::vector<std::string> items;
                if (TryParseStringArrayLines(text, items)) {
                    applyJsonValue(items);
                }
                if (auto locked = weakEditor.lock()) {
                    const auto refreshed = owner ? owner->ToJson() : nlohmann::ordered_json::object();
                    const auto& refreshedProperties = refreshed.contains("Properties")
                        ? refreshed.at("Properties")
                        : nlohmann::ordered_json::object();
                    const auto key = propertyClassName + "::" + propertyName;
                    if (refreshedProperties.contains(key)) {
                        locked->SetText(JoinLines(JsonValueToStringArray(refreshedProperties.at(key))));
                    }
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    return MakeInspectorPropertyRow(
        labelText,
        MakeInspectorReadOnlyField(DescribePropertyValue(property, objectJson)));
}

std::string ReflectionDetailsView::DescribePropertyValue(
    const Reflection::FPropertyDesc& property,
    const nlohmann::ordered_json& objectJson) const
{
    const std::string key = std::string(property.OwnerTypeName) + "::" + property.Name;
    const auto it = objectJson.find(key);
    if (it != objectJson.end()) {
        return JsonValueToString(it.value());
    }

    return property.Kind == Reflection::EPropertyKind::Struct
        ? EditorText("Details.StructPlaceholder", "{...}").Resolve()
        : EditorText("Details.Unavailable", "<unavailable>").Resolve();
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildStructPropertyEditorRow(
    const std::shared_ptr<ReflectableObject>& owner,
    const Reflection::FPropertyDesc& property,
    const std::string& propertyClassName,
    const std::string& propertyName,
    const nlohmann::ordered_json& propertyValueJson) const
{
    if (!owner || !property.GetConstReflectable) {
        return nullptr;
    }

    Reflection::FPropertyHandle propertyHandle(owner.get(), &property);
    const auto* nested = dynamic_cast<const ReflectableObject*>(
        property.GetConstReflectable(propertyHandle.GetConstPtr()));
    if (!nested) {
        return nullptr;
    }

    const std::string nestedTypeName = nested->GetTypeName();
    if (nestedTypeName == "FMargin") {
        const auto applyStructJson = [this, owner, propertyClassName, propertyName](const nlohmann::ordered_json& value) {
            if (!owner) {
                return false;
            }

            ReflectionDetailsView* mutableThis = const_cast<ReflectionDetailsView*>(this);
            if (mutableThis->OnPropertyValueCommitted.IsBound()) {
                mutableThis->OnPropertyValueCommitted.Broadcast(
                    *mutableThis,
                    owner,
                    propertyClassName,
                    propertyName,
                    value);
                return true;
            }

            auto serialized = owner->ToJson();
            serialized["Properties"][propertyClassName + "::" + propertyName] = value;
            owner->FromJson(serialized);
            mutableThis->RebuildPreservingViewState();
            mutableThis->OnPropertiesChanged.Broadcast(*mutableThis);
            return true;
        };

        const auto& nestedProperties = propertyValueJson.contains("Properties")
            ? propertyValueJson.at("Properties")
            : nlohmann::ordered_json::object();
        const auto readMarginComponent = [&nestedProperties](const char* key, float fallback) {
            return nestedProperties.contains(key) ? nestedProperties.at(key).get<float>() : fallback;
        };

        auto leftEditor = CreateInspectorTextEditor(FormatFloat(readMarginComponent("FMargin::Left", 0.0f)), "Left");
        auto rightEditor = CreateInspectorTextEditor(FormatFloat(readMarginComponent("FMargin::Right", 0.0f)), "Right");
        auto topEditor = CreateInspectorTextEditor(FormatFloat(readMarginComponent("FMargin::Top", 0.0f)), "Top");
        auto bottomEditor = CreateInspectorTextEditor(FormatFloat(readMarginComponent("FMargin::Bottom", 0.0f)), "Bottom");

        const auto syncEditors = [owner, propertyClassName, propertyName, weakLeft = std::weak_ptr<ImEditableText>(leftEditor), weakRight = std::weak_ptr<ImEditableText>(rightEditor), weakTop = std::weak_ptr<ImEditableText>(topEditor), weakBottom = std::weak_ptr<ImEditableText>(bottomEditor)]() {
            if (!owner) {
                return;
            }

            const auto refreshed = owner->ToJson();
            const auto& refreshedProperties = refreshed.contains("Properties")
                ? refreshed.at("Properties")
                : nlohmann::ordered_json::object();
            const auto key = propertyClassName + "::" + propertyName;
            if (!refreshedProperties.contains(key) ||
                !refreshedProperties.at(key).contains("Properties")) {
                return;
            }

            const auto& marginProperties = refreshedProperties.at(key).at("Properties");
            if (auto locked = weakLeft.lock()) {
                locked->SetText(FormatFloat(marginProperties.value("FMargin::Left", 0.0f)));
            }
            if (auto locked = weakRight.lock()) {
                locked->SetText(FormatFloat(marginProperties.value("FMargin::Right", 0.0f)));
            }
            if (auto locked = weakTop.lock()) {
                locked->SetText(FormatFloat(marginProperties.value("FMargin::Top", 0.0f)));
            }
            if (auto locked = weakBottom.lock()) {
                locked->SetText(FormatFloat(marginProperties.value("FMargin::Bottom", 0.0f)));
            }
        };

        const auto commitMargin = [applyStructJson, syncEditors, weakLeft = std::weak_ptr<ImEditableText>(leftEditor), weakRight = std::weak_ptr<ImEditableText>(rightEditor), weakTop = std::weak_ptr<ImEditableText>(topEditor), weakBottom = std::weak_ptr<ImEditableText>(bottomEditor)]() {
            auto lockedLeft = weakLeft.lock();
            auto lockedRight = weakRight.lock();
            auto lockedTop = weakTop.lock();
            auto lockedBottom = weakBottom.lock();
            if (!lockedLeft || !lockedRight || !lockedTop || !lockedBottom) {
                return;
            }

            float left = 0.0f;
            float right = 0.0f;
            float top = 0.0f;
            float bottom = 0.0f;
            if (!TryParseFloat(lockedLeft->GetText(), left) ||
                !TryParseFloat(lockedRight->GetText(), right) ||
                !TryParseFloat(lockedTop->GetText(), top) ||
                !TryParseFloat(lockedBottom->GetText(), bottom)) {
                syncEditors();
                return;
            }

            nlohmann::ordered_json marginJson;
            marginJson["Type"] = "FMargin";
            marginJson["Properties"] = {
                {"FMargin::Left", left},
                {"FMargin::Right", right},
                {"FMargin::Top", top},
                {"FMargin::Bottom", bottom}
            };
            applyStructJson(marginJson);
            syncEditors();
        };

        leftEditor->OnTextCommitted.AddLambda([commitMargin](ImEditableText&, const std::string&) { commitMargin(); });
        rightEditor->OnTextCommitted.AddLambda([commitMargin](ImEditableText&, const std::string&) { commitMargin(); });
        topEditor->OnTextCommitted.AddLambda([commitMargin](ImEditableText&, const std::string&) { commitMargin(); });
        bottomEditor->OnTextCommitted.AddLambda([commitMargin](ImEditableText&, const std::string&) { commitMargin(); });

        return MakeInspectorPropertyRow(
            propertyName,
            BuildCompactNumberEditors({
                {"L", leftEditor},
                {"R", rightEditor},
                {"T", topEditor},
                {"B", bottomEditor}}));
    }

    return nullptr;
}

std::shared_ptr<ReflectableObject> ReflectionDetailsView::ResolveNestedObject(
    const std::shared_ptr<ReflectableObject>& owner,
    const Reflection::FPropertyDesc& property) const
{
    if (!owner || property.Kind != Reflection::EPropertyKind::Struct || !property.GetReflectable) {
        return nullptr;
    }

    Reflection::FPropertyHandle propertyHandle(owner.get(), &property);
    ReflectableObject* nested = dynamic_cast<ReflectableObject*>(
        property.GetReflectable(propertyHandle.GetMutablePtr()));
    if (!nested) {
        return nullptr;
    }

    return std::shared_ptr<ReflectableObject>(
        owner,
        nested);
}

std::shared_ptr<ImOutlineView> ReflectionDetailsView::GetCurrentOutlineView() const
{
    return std::dynamic_pointer_cast<ImOutlineView>(GetRootWidget());
}

std::string ReflectionDetailsView::BuildCurrentStateKey() const
{
    if (!m_Target && !m_SlotTarget) {
        return "";
    }

    std::ostringstream stream;
    stream << reinterpret_cast<std::uintptr_t>(m_Target.get()) << ":"
           << reinterpret_cast<std::uintptr_t>(m_SlotTarget.get());
    return stream.str();
}

void ReflectionDetailsView::CaptureCurrentViewState()
{
    if (m_CurrentStateKey.empty()) {
        return;
    }

    auto outlineView = GetCurrentOutlineView();
    if (!outlineView) {
        return;
    }

    FInspectorViewState& state = m_ViewStatesByKey[m_CurrentStateKey];
    state.ScrollOffset = outlineView->GetScrollOffset();
    state.ExpandedByPath.clear();
    for (const auto& entry : m_CurrentPathItems) {
        if (entry.second != nullptr) {
            state.ExpandedByPath[entry.first] = entry.second->Expanded;
        }
    }
}

void ReflectionDetailsView::RestoreCurrentViewState()
{
    if (m_CurrentStateKey.empty()) {
        return;
    }

    auto outlineView = GetCurrentOutlineView();
    if (!outlineView) {
        return;
    }

    const auto stateIt = m_ViewStatesByKey.find(m_CurrentStateKey);
    if (stateIt == m_ViewStatesByKey.end()) {
        return;
    }

    outlineView->SetScrollOffset(stateIt->second.ScrollOffset);
}

bool ReflectionDetailsView::ResolveInitialExpandedState(const std::string& path, bool bDefaultExpanded) const
{
    const auto stateIt = m_ViewStatesByKey.find(m_CurrentStateKey);
    if (stateIt == m_ViewStatesByKey.end()) {
        return bDefaultExpanded;
    }

    const auto expandedIt = stateIt->second.ExpandedByPath.find(path);
    return expandedIt != stateIt->second.ExpandedByPath.end()
        ? expandedIt->second
        : bDefaultExpanded;
}

ImOutlineItem* ReflectionDetailsView::AddTrackedGroupItem(
    ImOutlineView& outlineView,
    ImOutlineItem* parentItem,
    const std::string& path,
    const std::string& title,
    bool bDefaultExpanded)
{
    ImOutlineItem* item = parentItem != nullptr
        ? outlineView.AddChildItem(parentItem, MakeSectionLabelWidget(title))
        : outlineView.AddRootItem(MakeSectionLabelWidget(title));
    if (!item) {
        return nullptr;
    }

    item->Expanded = ResolveInitialExpandedState(path, bDefaultExpanded);
    m_CurrentItemPaths[item] = path;
    m_CurrentPathItems[path] = item;
    return item;
}

} // namespace ImWidgetV4Editor
