#include "WidgetSerializer.h"

#include "WidgetFactory.h"

#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <stdexcept>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

json SerializeSlot(const ImSlot* slot)
{
    return slot ? slot->ToJson() : json();
}

json SerializeTabItems(const std::shared_ptr<ImTabView>& tabView)
{
    json tabItems = json::array();
    if (!tabView) {
        return tabItems;
    }

    for (int index = 0; index < tabView->GetTabCount(); ++index) {
        const FTabViewItem* tab = tabView->GetTab(index);
        if (tab == nullptr) {
            continue;
        }

        json item;
        item["Title"] = tab->Title;
        item["Enabled"] = tab->bEnabled;
        item["Closable"] = tab->bClosable;
        item["Dirty"] = tab->bDirty;
        item["Content"] = WidgetSerializer::SerializeWidgetTree(tab->Content);
        tabItems.push_back(std::move(item));
    }

    return tabItems;
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

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(parent)) {
        if (slotJson.is_object()) {
            const std::string role = slotJson.value("Role", "");
            if (role == "Header") {
                expandableBox->SetHeader(child);
                return true;
            }
            if (role == "Body") {
                expandableBox->SetBody(child);
                return true;
            }
        }

        if (!expandableBox->GetHeader()) {
            expandableBox->SetHeader(child);
        } else {
            expandableBox->SetBody(child);
        }
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

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        node["TabItems"] = SerializeTabItems(tabView);
        return node;
    }

    if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        node["Content"] = SerializeWidgetNode(button->GetContent());
        return node;
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        node["Header"] = SerializeWidgetNode(expandableBox->GetHeader());
        node["Body"] = SerializeWidgetNode(expandableBox->GetBody());
        return node;
    }

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
        if (widgetJson.is_null()) {
            result.bSuccess = true;
            result.Widget = nullptr;
            return result;
        }

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

        if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
            const int activeTabIndex = FindSerializedIntProperty(widgetJson, "::ActiveTabIndex", -1);
            if (widgetJson.contains("TabItems")) {
                const json& tabItemsJson = widgetJson.at("TabItems");
                if (!tabItemsJson.is_array()) {
                    result.ErrorMessage = "TabItems must be an array.";
                    return result;
                }

                tabView->ClearTabs();
                for (const auto& tabItemJson : tabItemsJson) {
                    if (!tabItemJson.is_object()) {
                        result.ErrorMessage = "Tab item must be an object.";
                        return result;
                    }

                    FWidgetSerializationResult contentResult;
                    if (tabItemJson.contains("Content")) {
                        contentResult = DeserializeWidgetNode(tabItemJson.at("Content"));
                        if (!contentResult.bSuccess) {
                            result.ErrorMessage = contentResult.ErrorMessage.empty()
                                ? "Failed to deserialize tab content."
                                : contentResult.ErrorMessage;
                            return result;
                        }
                    } else {
                        contentResult.bSuccess = true;
                    }

                    const std::string title = tabItemJson.value("Title", "Tab");
                    const int tabIndex = tabView->AddTab(title, contentResult.Widget);
                    if (tabIndex < 0) {
                        result.ErrorMessage = "Failed to create tab item.";
                        return result;
                    }

                    tabView->SetTabEnabled(tabIndex, tabItemJson.value("Enabled", true));
                    tabView->SetTabClosable(tabIndex, tabItemJson.value("Closable", false));
                    tabView->SetTabDirty(tabIndex, tabItemJson.value("Dirty", false));
                }
            }

            if (activeTabIndex >= 0) {
                tabView->SetActiveTab(activeTabIndex);
            }

            result.bSuccess = true;
            result.Widget = std::move(widget);
            return result;
        }

        if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
            if (widgetJson.contains("Content")) {
                FWidgetSerializationResult contentResult = DeserializeWidgetNode(widgetJson.at("Content"));
                if (!contentResult.bSuccess) {
                    result.ErrorMessage = contentResult.ErrorMessage.empty()
                        ? "Failed to deserialize button content."
                        : contentResult.ErrorMessage;
                    return result;
                }

                if (contentResult.Widget) {
                    button->SetContent(contentResult.Widget);
                }
            }

            result.bSuccess = true;
            result.Widget = std::move(widget);
            return result;
        }

        if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
            if (widgetJson.contains("Header")) {
                FWidgetSerializationResult headerResult = DeserializeWidgetNode(widgetJson.at("Header"));
                if (!headerResult.bSuccess) {
                    result.ErrorMessage = headerResult.ErrorMessage.empty()
                        ? "Failed to deserialize expandable header."
                        : headerResult.ErrorMessage;
                    return result;
                }
                expandableBox->SetHeader(headerResult.Widget);
            }

            if (widgetJson.contains("Body")) {
                FWidgetSerializationResult bodyResult = DeserializeWidgetNode(widgetJson.at("Body"));
                if (!bodyResult.bSuccess) {
                    result.ErrorMessage = bodyResult.ErrorMessage.empty()
                        ? "Failed to deserialize expandable body."
                        : bodyResult.ErrorMessage;
                    return result;
                }
                expandableBox->SetBody(bodyResult.Widget);
            }

            result.bSuccess = true;
            result.Widget = std::move(widget);
            return result;
        }

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
