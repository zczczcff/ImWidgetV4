#pragma once

#include "../serialization/DocumentFormat.h"

#include <memory>
#include <vector>

namespace ImWidgetV4 {
class ImButton;
class ImExpandableBox;
class ImTabView;
class ImWidget;
}

namespace ImWidgetV4Editor::LogicalWidgetTree {

int FindTabContentIndex(
    const std::shared_ptr<ImWidgetV4::ImTabView>& tabView,
    const std::shared_ptr<ImWidgetV4::ImWidget>& content);

std::size_t GetLogicalChildCount(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);

std::shared_ptr<ImWidgetV4::ImWidget> GetLogicalChildAt(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    std::size_t childIndex);

const char* GetLogicalChildRoleName(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    std::size_t childIndex);

std::vector<std::shared_ptr<ImWidgetV4::ImWidget>> GetLogicalChildren(
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget);

int FindLogicalChildIndex(
    const std::shared_ptr<ImWidgetV4::ImWidget>& parent,
    const std::shared_ptr<ImWidgetV4::ImWidget>& child);

std::shared_ptr<ImWidgetV4::ImWidget> FindLogicalParentRecursive(
    const std::shared_ptr<ImWidgetV4::ImWidget>& current,
    const std::shared_ptr<ImWidgetV4::ImWidget>& target);

json* ResolveLogicalChildJson(
    json& widgetJson,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    std::size_t childIndex);

const json* ResolveLogicalChildJson(
    const json& widgetJson,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    std::size_t childIndex);

} // namespace ImWidgetV4Editor::LogicalWidgetTree
