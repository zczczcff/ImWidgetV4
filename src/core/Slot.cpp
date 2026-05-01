#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/core/Widget.h>
#include <algorithm>

namespace ImWidgetV4 {

// ==================== ImSlot ====================

ImSlot::ImSlot(ImWidget* content, ImWidget* parent)
    : m_Content(content)
    , m_Parent(parent)
    , m_SlotPosition(0.0f, 0.0f)
    , m_SlotSize(0.0f, 0.0f)
{
}

ImSlot::~ImSlot() {
    // 不删除 content 和 parent，它们由外部管理
}

void ImSlot::ApplyLayout() {
    if (!m_Content) {
        return;
    }

    // 基础 Slot：子控件占据整个 Slot 空间
    FGeometry geometry;
    geometry.Position = m_SlotPosition;
    geometry.Size = m_SlotSize;
    m_Content->SetGeometry(geometry);
}

void ImSlot::Render(const FPaintContext& paintContext) {
    if (m_Content && m_Content->IsVisible()) {
        m_Content->Paint(paintContext);
    }
}

// ==================== ImPaddingSlot ====================

ImPaddingSlot::ImPaddingSlot(ImWidget* content, ImWidget* parent)
    : ImSlot(content, parent)
    , PaddingLeft(0.0f)
    , PaddingRight(0.0f)
    , PaddingTop(0.0f)
    , PaddingBottom(0.0f)
{
}

void ImPaddingSlot::ApplyLayout() {
    if (!m_Content) {
        return;
    }

    // 计算内边距后的可用空间
    FVector2 rectMin(
        m_SlotPosition.X + PaddingLeft,
        m_SlotPosition.Y + PaddingTop
    );

    FVector2 rectMax(
        m_SlotPosition.X + m_SlotSize.X - PaddingRight,
        m_SlotPosition.Y + m_SlotSize.Y - PaddingBottom
    );

    // 计算子控件的实际大小
    FVector2 widgetSize(
        std::max(0.0f, rectMax.X - rectMin.X),
        std::max(0.0f, rectMax.Y - rectMin.Y)
    );

    // 设置子控件的几何信息
    FGeometry geometry;
    geometry.Position = rectMin;
    geometry.Size = widgetSize;
    m_Content->SetGeometry(geometry);
}

} // namespace ImWidgetV4
