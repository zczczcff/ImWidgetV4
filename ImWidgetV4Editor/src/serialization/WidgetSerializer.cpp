#include "WidgetSerializer.h"

#include "WidgetFactory.h"

#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <stdexcept>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

json SerializeSlot(const ImSlot* slot)
{
    return slot ? slot->ToJson() : json();
}

bool TryApplySlotToParent(
    const std::shared_ptr<ImWidget>& parent,
    const std::shared_ptr<ImWidget>& child,
    const json& slotJson,
    std::string& outError)
{
    if (!parent || !child) {
        outError = "Invalid parent/child while applying slot.";
        return false;
    }

    if (auto verticalBox = std::dynamic_pointer_cast<ImVerticalBox>(parent)) {
        verticalBox->AddChild(child);
        if (!slotJson.is_null()) {
            if (auto* slot = verticalBox->GetSlotForChild(child)) {
                slot->FromJson(slotJson);
            }
        }
        return true;
    }

    if (auto horizontalBox = std::dynamic_pointer_cast<ImHorizontalBox>(parent)) {
        horizontalBox->AddChild(child);
        if (!slotJson.is_null()) {
            if (auto* slot = horizontalBox->GetSlotForChild(child)) {
                slot->FromJson(slotJson);
            }
        }
        return true;
    }

    if (auto canvasPanel = std::dynamic_pointer_cast<ImCanvasPanel>(parent)) {
        canvasPanel->AddChild(child);
        if (!slotJson.is_null()) {
            if (auto* slot = canvasPanel->GetSlotForChild(child)) {
                slot->FromJson(slotJson);
            }
        }
        return true;
    }

    if (auto scrollBox = std::dynamic_pointer_cast<ImScrollBox>(parent)) {
        scrollBox->SetContent(child);
        return true;
    }

    if (auto button = std::dynamic_pointer_cast<ImButton>(parent)) {
        button->SetContent(child);
        return true;
    }

    parent->AddChild(child);
    return true;
}

} // namespace

json WidgetSerializer::SerializeWidgetTree(const std::shared_ptr<ImWidget>& widget)
{
    return SerializeWidgetNode(widget);
}

json WidgetSerializer::SerializeWidgetNode(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return json();
    }

    json node = widget->ToJson();
    node["Children"] = json::array();

    const auto& children = widget->GetChildren();
    const auto panel = std::dynamic_pointer_cast<ImPanelWidget>(widget);

    for (size_t childIndex = 0; childIndex < children.size(); ++childIndex) {
        const auto& child = children[childIndex];
        if (!child) {
            continue;
        }

        json childNode = SerializeWidgetNode(child);
        if (panel) {
            childNode["Slot"] = SerializeSlot(panel->GetSlotAt(static_cast<int>(childIndex)));
        } else {
            childNode["Slot"] = json();
        }

        node["Children"].push_back(childNode);
    }

    return node;
}

FWidgetSerializationResult WidgetSerializer::DeserializeWidgetTree(const json& widgetJson)
{
    return DeserializeWidgetNode(widgetJson);
}

FWidgetSerializationResult WidgetSerializer::DeserializeWidgetNode(const json& widgetJson)
{
    FWidgetSerializationResult result;

    try {
        if (!widgetJson.is_object()) {
            result.ErrorMessage = "Widget JSON node must be an object.";
            return result;
        }

        if (!widgetJson.contains("Type")) {
            result.ErrorMessage = "Widget JSON node is missing Type.";
            return result;
        }

        const std::string typeName = widgetJson.at("Type").get<std::string>();
        auto widget = WidgetFactory::Get().CreateWidget(typeName);
        if (!widget) {
            result.ErrorMessage = "Unsupported widget type: " + typeName;
            return result;
        }

        widget->FromJson(widgetJson);

        if (widgetJson.contains("Children")) {
            const json& childrenJson = widgetJson.at("Children");
            if (!childrenJson.is_array()) {
                result.ErrorMessage = "Children must be an array.";
                return result;
            }

            for (const auto& childJson : childrenJson) {
                FWidgetSerializationResult childResult = DeserializeWidgetNode(childJson);
                if (!childResult.bSuccess || !childResult.Widget) {
                    result.ErrorMessage = childResult.ErrorMessage.empty()
                        ? "Failed to deserialize child widget."
                        : childResult.ErrorMessage;
                    return result;
                }

                const json& slotJson = childJson.contains("Slot") ? childJson.at("Slot") : json();
                if (!TryApplySlotToParent(widget, childResult.Widget, slotJson, result.ErrorMessage)) {
                    return result;
                }
            }
        }

        result.bSuccess = true;
        result.Widget = std::move(widget);
        return result;
    } catch (const std::exception& e) {
        result.ErrorMessage = e.what();
        return result;
    }
}

} // namespace ImWidgetV4Editor
