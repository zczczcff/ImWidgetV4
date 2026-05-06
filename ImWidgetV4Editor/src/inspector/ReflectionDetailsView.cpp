#include "ReflectionDetailsView.h"
#include "PropertyEditorWidgets.h"

#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor::PropertyEditorWidgets;

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

std::string FormatColor(const FColor& color)
{
    const int r = static_cast<int>(std::round(color.R * 255.0f));
    const int g = static_cast<int>(std::round(color.G * 255.0f));
    const int b = static_cast<int>(std::round(color.B * 255.0f));
    const int a = static_cast<int>(std::round(color.A * 255.0f));
    return std::to_string(r) + ", " + std::to_string(g) + ", " + std::to_string(b) + ", " + std::to_string(a);
}

std::string FormatVec2(const FVector2& value)
{
    return FormatFloat(value.X) + ", " + FormatFloat(value.Y);
}

std::shared_ptr<ImWidget> BuildWidgetMetadataRows(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return nullptr;
    }

    auto rows = std::make_shared<ImVerticalBox>();
    rows->SetSpacing(6.0f);

    const FGeometry geometry = widget->GetGeometry();
    std::string parentLabel = "<root>";
    if (auto parent = widget->GetParent()) {
        parentLabel = parent->GetTypeName();
        if (!parent->GetName().empty()) {
            parentLabel += " [" + parent->GetName() + "]";
        }
    }

    rows->AddChild(MakeInspectorPropertyRow("Type", MakeInspectorReadOnlyField(widget->GetTypeName())));
    rows->AddChild(MakeInspectorPropertyRow(
        "Name",
        MakeInspectorReadOnlyField(widget->GetName().empty() ? "<unnamed>" : widget->GetName())));
    rows->AddChild(MakeInspectorPropertyRow("Position", MakeInspectorReadOnlyField(FormatVec2(geometry.Position))));
    rows->AddChild(MakeInspectorPropertyRow("Size", MakeInspectorReadOnlyField(FormatVec2(geometry.Size))));
    rows->AddChild(MakeInspectorPropertyRow("Parent", MakeInspectorReadOnlyField(parentLabel)));
    return rows;
}

std::shared_ptr<ImWidget> MakeSectionLabelWidget(const std::string& title)
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(title);
    label->SetWrapText(false);
    label->SetFontSize(14.0f);
    label->SetTextColor(FColor::FromBytes(242, 246, 250));
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

    m_Target = target;
    m_SlotTarget = slotTarget;
    Rebuild();
}

void ReflectionDetailsView::SetTarget(const std::shared_ptr<ReflectableObject>& target)
{
    SetTargets(target, m_SlotTarget);
}

void ReflectionDetailsView::SetSlotTarget(const std::shared_ptr<ImSlot>& slotTarget)
{
    SetTargets(m_Target, slotTarget);
}

ImWidget::Ptr ReflectionDetailsView::RebuildWidget()
{
    if (!m_Target && !m_SlotTarget) {
        return BuildEmptyState();
    }

    auto outlineView = std::make_shared<ImOutlineView>();
    outlineView->SetSupportsKeyboardFocus(true);

    FOutlineViewStyle style = outlineView->GetStyle();
    style.Padding = FMargin(0.0f);
    style.RowPadding = FMargin(6.0f, 8.0f, 4.0f, 4.0f);
    style.BackgroundColor = FColor::FromBytes(24, 28, 34);
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.IndentWidth = 16.0f;
    style.IndicatorSize = 10.0f;
    style.IndicatorSpacing = 6.0f;
    style.RowMinHeight = 30.0f;
    style.MinDesiredSize = FVector2(280.0f, 220.0f);
    outlineView->SetStyle(style);

    if (m_Target) {
        if (auto widget = std::dynamic_pointer_cast<ImWidget>(m_Target)) {
            BuildCommonSection(*outlineView, widget);
            BuildObjectSection(*outlineView, m_Target, "Properties");
        } else {
            BuildObjectSection(*outlineView, m_Target, m_Target->GetTypeName());
        }
    }
    if (m_SlotTarget) {
        BuildObjectSection(*outlineView, m_SlotTarget, "Slot");
    }

    return outlineView;
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildEmptyState() const
{
    auto outlineView = std::make_shared<ImOutlineView>();
    FOutlineViewStyle style = outlineView->GetStyle();
    style.Padding = FMargin(0.0f);
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    outlineView->SetStyle(style);

    ImOutlineItem* rootItem = outlineView->AddRootItem(MakeSectionLabelWidget("Details"));
    if (rootItem) {
        rootItem->Expanded = true;
        outlineView->AddChildItem(
            rootItem,
            MakeText(
                "Select a widget in the designer surface to inspect its reflected properties.",
                13.0f,
                FColor::FromBytes(178, 188, 201)));
    }
    return outlineView;
}

void ReflectionDetailsView::BuildCommonSection(
    ImOutlineView& outlineView,
    const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget) {
        return;
    }

    ImOutlineItem* sectionItem = outlineView.AddRootItem(MakeSectionLabelWidget("Common"));
    if (!sectionItem) {
        return;
    }

    sectionItem->Expanded = true;

    const FGeometry geometry = widget->GetGeometry();
    std::string parentLabel = "<root>";
    if (auto parent = widget->GetParent()) {
        parentLabel = parent->GetTypeName();
        if (!parent->GetName().empty()) {
            parentLabel += " [" + parent->GetName() + "]";
        }
    }

    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow("Type", MakeInspectorReadOnlyField(widget->GetTypeName())));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow(
        "Name",
        MakeInspectorReadOnlyField(widget->GetName().empty() ? "<unnamed>" : widget->GetName())));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow("Position", MakeInspectorReadOnlyField(FormatVec2(geometry.Position))));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow("Size", MakeInspectorReadOnlyField(FormatVec2(geometry.Size))));
    outlineView.AddChildItem(sectionItem, MakeInspectorPropertyRow("Parent", MakeInspectorReadOnlyField(parentLabel)));
}

void ReflectionDetailsView::BuildObjectSection(
    ImOutlineView& outlineView,
    const std::shared_ptr<ReflectableObject>& object,
    const std::string& title) const
{
    if (!object) {
        return;
    }

    ImOutlineItem* sectionItem = outlineView.AddRootItem(MakeSectionLabelWidget(title));
    if (!sectionItem) {
        return;
    }

    sectionItem->Expanded = true;
    BuildPropertyItems(outlineView, sectionItem, object);
}

void ReflectionDetailsView::BuildPropertyItems(
    ImOutlineView& outlineView,
    ImOutlineItem* parentItem,
    const std::shared_ptr<ReflectableObject>& object) const
{
    if (!parentItem || !object) {
        return;
    }

    const auto objectJson = object->ToJson();
    const auto& propertyJson = objectJson.contains("Properties")
        ? objectJson.at("Properties")
        : nlohmann::ordered_json::object();
    const auto properties = object->GetAllPropertiesOrdered();
    for (const auto& property : properties) {
        if (std::dynamic_pointer_cast<ImWidget>(object) &&
            property.GetClassName() == "ImWidget" &&
            property.GetNameString() == "Name") {
            continue;
        }

        auto nestedObject = ResolveNestedObject(object, property);
        if (nestedObject) {
            ImOutlineItem* groupItem = outlineView.AddChildItem(parentItem, MakeSectionLabelWidget(property.GetNameString()));
            if (groupItem) {
                groupItem->Expanded = true;
                BuildPropertyItems(outlineView, groupItem, nestedObject);
            }
            continue;
        }

        outlineView.AddChildItem(
            parentItem,
            BuildPropertyEditorRow(
                object,
                property,
                propertyJson));
    }
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildPropertyEditorRow(
    const std::shared_ptr<ReflectableObject>& owner,
    const ReflectableObject::ROPProperty& property,
    const nlohmann::ordered_json& objectJson) const
{
    const std::string propertyName = property.GetNameString();
    const std::string propertyClassName = property.GetClassName();
    const std::string labelText = propertyName;
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
        mutableThis->Rebuild();
        mutableThis->OnPropertiesChanged.Broadcast(*mutableThis);
        return true;
    };

    if (property.GetType() == PropertyType::Bool) {
        auto checkBox = std::make_shared<ImCheckBox>();
        checkBox->SetLabel(labelText);
        checkBox->SetChecked(property.GetValue<bool>());
        checkBox->OnCheckStateChanged.AddLambda(
            [applyJsonValue](ImCheckBox&, bool checked) {
                applyJsonValue(checked);
            });
        return checkBox;
    }

    if (property.GetType() == PropertyType::String) {
        auto editor = std::make_shared<ImEditableText>();
        ApplyInspectorEditableTextStyle(*editor, false);
        editor->SetText(property.GetValue<std::string>());
        editor->OnTextCommitted.AddLambda(
            [applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                applyJsonValue(text);
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(text);
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.GetType() == PropertyType::Int) {
        auto editor = std::make_shared<ImEditableText>();
        ApplyInspectorEditableTextStyle(*editor, false);
        editor->SetText(std::to_string(property.GetValue<int>()));
        editor->OnTextCommitted.AddLambda(
            [property, applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                int value = 0;
                if (TryParseInt(text, value)) {
                    applyJsonValue(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(std::to_string(property.GetObject()->GetProperty(property.GetNameString(), property.GetClassName()).GetValue<int>()));
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.GetType() == PropertyType::Float) {
        auto editor = std::make_shared<ImEditableText>();
        ApplyInspectorEditableTextStyle(*editor, false);
        editor->SetText(FormatFloat(property.GetValue<float>()));
        editor->OnTextCommitted.AddLambda(
            [property, applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                float value = 0.0f;
                if (TryParseFloat(text, value)) {
                    applyJsonValue(value);
                }
                if (auto locked = weakEditor.lock()) {
                    locked->SetText(FormatFloat(property.GetObject()->GetProperty(property.GetNameString(), property.GetClassName()).GetValue<float>()));
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.GetType() == PropertyType::Enum) {
        auto editor = std::make_shared<ImComboBox>();
        ApplyInspectorComboBoxStyle(*editor);
        auto optionalProperty = owner->ToOptionalProperty(property);
        const auto& options = optionalProperty.GetOptionList();
        editor->SetItems(options);

        const std::string currentOption = optionalProperty.GetOptionString();
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

                auto reloaded = owner->GetProperty(propertyName, propertyClassName);
                if (!reloaded.IsValid()) {
                    return;
                }

                auto optional = owner->ToOptionalProperty(reloaded);
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

                mutableThis->Rebuild();
                mutableThis->OnPropertiesChanged.Broadcast(*mutableThis);
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.GetType() == PropertyType::Color) {
        auto editor = std::make_shared<ImEditableText>();
        ApplyInspectorEditableTextStyle(*editor, false);
        const std::string currentText = DescribePropertyValue(property, objectJson);
        editor->SetText(currentText);
        editor->OnTextCommitted.AddLambda(
            [owner, propertyName, propertyClassName, applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                FColor value;
                if (TryParseColor(text, value)) {
                    applyJsonValue(nlohmann::ordered_json::array({
                        static_cast<int>(std::round(value.R * 255.0f)),
                        static_cast<int>(std::round(value.G * 255.0f)),
                        static_cast<int>(std::round(value.B * 255.0f)),
                        static_cast<int>(std::round(value.A * 255.0f))}));
                }
                if (auto locked = weakEditor.lock()) {
                    const auto refreshed = owner ? owner->ToJson() : nlohmann::ordered_json::object();
                    const auto& refreshedProperties = refreshed.contains("Properties")
                        ? refreshed.at("Properties")
                        : nlohmann::ordered_json::object();
                    const auto key = propertyClassName + "::" + propertyName;
                    if (refreshedProperties.contains(key)) {
                        locked->SetText(JsonValueToString(refreshedProperties.at(key)));
                    }
                }
            });
        return MakeInspectorPropertyRow(labelText, editor);
    }

    if (property.GetType() == PropertyType::Vec2) {
        auto editor = std::make_shared<ImEditableText>();
        ApplyInspectorEditableTextStyle(*editor, false);
        editor->SetText(DescribePropertyValue(property, objectJson));
        editor->OnTextCommitted.AddLambda(
            [owner, propertyName, propertyClassName, applyJsonValue, weakEditor = std::weak_ptr<ImEditableText>(editor)](ImEditableText&, const std::string& text) {
                FVector2 value;
                if (TryParseVec2(text, value)) {
                    applyJsonValue(nlohmann::ordered_json::array({value.X, value.Y}));
                }
                if (auto locked = weakEditor.lock()) {
                    const auto refreshed = owner ? owner->ToJson() : nlohmann::ordered_json::object();
                    const auto& refreshedProperties = refreshed.contains("Properties")
                        ? refreshed.at("Properties")
                        : nlohmann::ordered_json::object();
                    const auto key = propertyClassName + "::" + propertyName;
                    if (refreshedProperties.contains(key)) {
                        locked->SetText(JsonValueToString(refreshedProperties.at(key)));
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
    const ReflectableObject::ROPProperty& property,
    const nlohmann::ordered_json& objectJson) const
{
    const std::string key = property.GetClassName() + "::" + property.GetNameString();
    const auto it = objectJson.find(key);
    if (it != objectJson.end()) {
        return JsonValueToString(it.value());
    }

    return property.GetType() == PropertyType::Struct ? "{...}" : "<unavailable>";
}

std::shared_ptr<ReflectableObject> ReflectionDetailsView::ResolveNestedObject(
    const std::shared_ptr<ReflectableObject>& owner,
    const ReflectableObject::ROPProperty& property) const
{
    if (property.GetType() != PropertyType::Struct) {
        return nullptr;
    }

    const ReflectableObject* nested = property.GetConstPointer<ReflectableObject>();
    if (!nested) {
        return nullptr;
    }

    return std::shared_ptr<ReflectableObject>(
        owner,
        const_cast<ReflectableObject*>(nested));
}

} // namespace ImWidgetV4Editor
