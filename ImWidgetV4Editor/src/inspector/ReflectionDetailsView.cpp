#include "ReflectionDetailsView.h"

#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <sstream>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

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

} // namespace

ReflectionDetailsView::ReflectionDetailsView()
    : ImUserWidget()
{
    SetHitTestVisible(true);
}

void ReflectionDetailsView::SetTarget(const std::shared_ptr<ReflectableObject>& target)
{
    if (m_Target == target) {
        return;
    }

    m_Target = target;
    Rebuild();
}

ImWidget::Ptr ReflectionDetailsView::RebuildWidget()
{
    if (!m_Target) {
        return BuildEmptyState();
    }

    auto root = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style = root->GetStyle();
    style.Padding = FMargin(10.0f);
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    root->SetStyle(style);

    auto stack = std::make_shared<ImVerticalBox>();
    stack->SetSpacing(10.0f);
    stack->AddChild(BuildDetailsForObject(m_Target, m_Target->GetTypeName()));
    root->SetContent(stack);
    return root;
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildEmptyState() const
{
    auto box = std::make_shared<ImVerticalBox>();
    box->SetSpacing(8.0f);
    box->AddChild(MakeText("Selection", 16.0f, FColor::FromBytes(235, 240, 247)), FMargin(6.0f));
    box->AddChild(
        MakeText(
            "Select a widget in the designer surface to inspect its reflected properties.",
            13.0f,
            FColor::FromBytes(178, 188, 201)),
        FMargin(6.0f, 6.0f, 0.0f, 6.0f));
    return box;
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildDetailsForObject(
    const std::shared_ptr<ReflectableObject>& object,
    const std::string& title) const
{
    auto expandable = std::make_shared<ImExpandableBox>();
    expandable->SetExpanded(true);
    expandable->SetHeader(MakeText(title, 14.0f, FColor::FromBytes(242, 246, 250)));
    expandable->SetBody(BuildPropertyRows(*object, 0));
    return expandable;
}

std::shared_ptr<ImWidget> ReflectionDetailsView::BuildPropertyRows(
    ReflectableObject& object,
    int indentLevel) const
{
    auto rows = std::make_shared<ImVerticalBox>();
    rows->SetSpacing(6.0f);

    const auto objectJson = object.ToJson();
    const auto& propertyJson = objectJson.contains("Properties")
        ? objectJson.at("Properties")
        : nlohmann::ordered_json::object();
    const auto properties = object.GetAllPropertiesOrdered();
    for (const auto& property : properties) {
        auto nestedObject = ResolveNestedObject(property);
        if (nestedObject) {
            rows->AddChild(
                BuildDetailsForObject(
                    nestedObject,
                    std::string(indentLevel * 2, ' ') + property.GetNameString()),
                FMargin(12.0f, 0.0f, 4.0f, 0.0f));
            continue;
        }

        auto line = std::make_shared<ImHorizontalBox>();
        line->SetSpacing(8.0f);
        line->AddChild(
            MakeText(
                std::string(indentLevel * 2, ' ') + property.GetNameString() + ":",
                13.0f,
                FColor::FromBytes(224, 230, 237)),
            FMargin(2.0f));
        line->AddChild(
            MakeText(
                DescribePropertyValue(property, propertyJson),
                13.0f,
                FColor::FromBytes(167, 197, 255)),
            FMargin(2.0f));
        rows->AddChild(line);
    }

    return rows;
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
        m_Target,
        const_cast<ReflectableObject*>(nested));
}

} // namespace ImWidgetV4Editor
