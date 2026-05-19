#include "LogicalWidgetTree.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TitleBar.h>

namespace ImWidgetV4Editor::LogicalWidgetTree {

using namespace ImWidgetV4;

int FindTabContentIndex(const std::shared_ptr<ImTabView>& tabView, const std::shared_ptr<ImWidget>& content)
{
    if (!tabView || !content) {
        return -1;
    }

    for (int index = 0; index < tabView->GetTabCount(); ++index) {
        const FTabViewItem* tab = tabView->GetTab(index);
        if (tab && tab->Content == content) {
            return index;
        }
    }

    return -1;
}

std::size_t GetLogicalChildCount(const std::shared_ptr<ImWidget>& widget)
{
    if (!widget) {
        return 0;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        return static_cast<std::size_t>(tabView->GetTabCount());
    }

    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(widget)) {
        return titleBar->GetLeadingItemCount() + titleBar->GetTrailingItemCount();
    }

    return widget->GetChildren().size();
}

std::shared_ptr<ImWidget> GetLogicalChildAt(const std::shared_ptr<ImWidget>& widget, std::size_t childIndex)
{
    if (!widget) {
        return nullptr;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(widget)) {
        const FTabViewItem* tab = tabView->GetTab(static_cast<int>(childIndex));
        return tab ? tab->Content : nullptr;
    }

    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(widget)) {
        const std::size_t leadingCount = titleBar->GetLeadingItemCount();
        if (childIndex < leadingCount) {
            return titleBar->GetLeadingItemAt(childIndex);
        }
        return titleBar->GetTrailingItemAt(childIndex - leadingCount);
    }

    if (auto button = std::dynamic_pointer_cast<ImButton>(widget)) {
        return childIndex == 0 ? button->GetContent() : nullptr;
    }

    if (auto expandableBox = std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        if (childIndex == 0) {
            return expandableBox->GetHeader();
        }
        if (childIndex == 1) {
            return expandableBox->GetBody();
        }
        return nullptr;
    }

    const auto& children = widget->GetChildren();
    if (childIndex >= children.size()) {
        return nullptr;
    }

    return children[childIndex];
}

const char* GetLogicalChildRoleName(const std::shared_ptr<ImWidget>& widget, std::size_t childIndex)
{
    if (!widget) {
        return nullptr;
    }

    if (std::dynamic_pointer_cast<ImTabView>(widget)) {
        return "Tab";
    }

    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(widget)) {
        return childIndex < titleBar->GetLeadingItemCount() ? "Leading" : "Trailing";
    }

    if (std::dynamic_pointer_cast<ImButton>(widget)) {
        return childIndex == 0 ? "Content" : nullptr;
    }

    if (std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        if (childIndex == 0) {
            return "Header";
        }
        if (childIndex == 1) {
            return "Body";
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<ImWidget>> GetLogicalChildren(const std::shared_ptr<ImWidget>& widget)
{
    std::vector<std::shared_ptr<ImWidget>> children;
    const std::size_t childCount = GetLogicalChildCount(widget);
    children.reserve(childCount);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        if (std::shared_ptr<ImWidget> child = GetLogicalChildAt(widget, childIndex)) {
            children.push_back(child);
        }
    }
    return children;
}

int FindLogicalChildIndex(const std::shared_ptr<ImWidget>& parent, const std::shared_ptr<ImWidget>& child)
{
    if (!parent || !child) {
        return -1;
    }

    if (auto tabView = std::dynamic_pointer_cast<ImTabView>(parent)) {
        return FindTabContentIndex(tabView, child);
    }

    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(parent)) {
        for (std::size_t index = 0; index < titleBar->GetLeadingItemCount(); ++index) {
            if (titleBar->GetLeadingItemAt(index) == child) {
                return static_cast<int>(index);
            }
        }
        const std::size_t leadingCount = titleBar->GetLeadingItemCount();
        for (std::size_t index = 0; index < titleBar->GetTrailingItemCount(); ++index) {
            if (titleBar->GetTrailingItemAt(index) == child) {
                return static_cast<int>(leadingCount + index);
            }
        }
        return -1;
    }

    const auto& children = parent->GetChildren();
    for (std::size_t index = 0; index < children.size(); ++index) {
        if (children[index] == child) {
            return static_cast<int>(index);
        }
    }

    return -1;
}

std::shared_ptr<ImWidget> FindLogicalParentRecursive(
    const std::shared_ptr<ImWidget>& current,
    const std::shared_ptr<ImWidget>& target)
{
    if (!current || !target) {
        return nullptr;
    }

    const std::size_t childCount = GetLogicalChildCount(current);
    for (std::size_t childIndex = 0; childIndex < childCount; ++childIndex) {
        std::shared_ptr<ImWidget> child = GetLogicalChildAt(current, childIndex);
        if (!child) {
            continue;
        }

        if (child == target) {
            return current;
        }

        if (std::shared_ptr<ImWidget> recursiveParent = FindLogicalParentRecursive(child, target)) {
            return recursiveParent;
        }
    }

    return nullptr;
}

namespace {

json* ResolveMutableLogicalChildJson(
    json& widgetJson,
    const std::shared_ptr<ImWidget>& widget,
    std::size_t childIndex)
{
    if (!widgetJson.is_object()) {
        return nullptr;
    }

    if (std::dynamic_pointer_cast<ImTabView>(widget)) {
        if (!widgetJson.contains("TabItems") || !widgetJson["TabItems"].is_array()) {
            return nullptr;
        }

        json& tabItems = widgetJson["TabItems"];
        if (childIndex >= tabItems.size()) {
            return nullptr;
        }

        json& tabItem = tabItems[childIndex];
        if (!tabItem.is_object()) {
            return nullptr;
        }

        if (!tabItem.contains("Content")) {
            tabItem["Content"] = json();
        }
        return &tabItem["Content"];
    }

    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(widget)) {
        const std::size_t leadingCount = titleBar->GetLeadingItemCount();
        const char* fieldName = childIndex < leadingCount ? "LeadingItems" : "TrailingItems";
        const std::size_t localIndex = childIndex < leadingCount ? childIndex : (childIndex - leadingCount);
        if (!widgetJson.contains(fieldName) || !widgetJson[fieldName].is_array()) {
            return nullptr;
        }

        json& items = widgetJson[fieldName];
        if (localIndex >= items.size()) {
            return nullptr;
        }
        return &items[localIndex];
    }

    if (std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        if (childIndex == 0) {
            if (!widgetJson.contains("Header")) {
                widgetJson["Header"] = json();
            }
            return &widgetJson["Header"];
        }

        if (childIndex == 1) {
            if (!widgetJson.contains("Body")) {
                widgetJson["Body"] = json();
            }
            return &widgetJson["Body"];
        }

        return nullptr;
    }

    if (std::dynamic_pointer_cast<ImButton>(widget)) {
        if (childIndex != 0) {
            return nullptr;
        }

        if (!widgetJson.contains("Content")) {
            widgetJson["Content"] = json();
        }
        return &widgetJson["Content"];
    }

    if (!widgetJson.contains("Children") || !widgetJson["Children"].is_array()) {
        return nullptr;
    }

    json& children = widgetJson["Children"];
    if (childIndex >= children.size()) {
        return nullptr;
    }

    return &children[childIndex];
}

const json* ResolveConstLogicalChildJson(
    const json& widgetJson,
    const std::shared_ptr<ImWidget>& widget,
    std::size_t childIndex)
{
    if (!widgetJson.is_object()) {
        return nullptr;
    }

    if (std::dynamic_pointer_cast<ImTabView>(widget)) {
        if (!widgetJson.contains("TabItems") || !widgetJson["TabItems"].is_array()) {
            return nullptr;
        }

        const json& tabItems = widgetJson["TabItems"];
        if (childIndex >= tabItems.size()) {
            return nullptr;
        }

        const json& tabItem = tabItems[childIndex];
        if (!tabItem.is_object() || !tabItem.contains("Content")) {
            return nullptr;
        }

        return &tabItem["Content"];
    }

    if (auto titleBar = std::dynamic_pointer_cast<ImTitleBar>(widget)) {
        const std::size_t leadingCount = titleBar->GetLeadingItemCount();
        const char* fieldName = childIndex < leadingCount ? "LeadingItems" : "TrailingItems";
        const std::size_t localIndex = childIndex < leadingCount ? childIndex : (childIndex - leadingCount);
        if (!widgetJson.contains(fieldName) || !widgetJson[fieldName].is_array()) {
            return nullptr;
        }

        const json& items = widgetJson[fieldName];
        if (localIndex >= items.size()) {
            return nullptr;
        }
        return &items[localIndex];
    }

    if (std::dynamic_pointer_cast<ImExpandableBox>(widget)) {
        if (childIndex == 0) {
            return widgetJson.contains("Header") ? &widgetJson["Header"] : nullptr;
        }

        if (childIndex == 1) {
            return widgetJson.contains("Body") ? &widgetJson["Body"] : nullptr;
        }

        return nullptr;
    }

    if (std::dynamic_pointer_cast<ImButton>(widget)) {
        if (childIndex != 0) {
            return nullptr;
        }

        return widgetJson.contains("Content") ? &widgetJson["Content"] : nullptr;
    }

    if (!widgetJson.contains("Children") || !widgetJson["Children"].is_array()) {
        return nullptr;
    }

    const json& children = widgetJson["Children"];
    if (childIndex >= children.size()) {
        return nullptr;
    }

    return &children[childIndex];
}

} // namespace

json* ResolveLogicalChildJson(json& widgetJson, const std::shared_ptr<ImWidget>& widget, std::size_t childIndex)
{
    return ResolveMutableLogicalChildJson(widgetJson, widget, childIndex);
}

const json* ResolveLogicalChildJson(
    const json& widgetJson,
    const std::shared_ptr<ImWidget>& widget,
    std::size_t childIndex)
{
    return ResolveConstLogicalChildJson(widgetJson, widget, childIndex);
}

} // namespace ImWidgetV4Editor::LogicalWidgetTree
