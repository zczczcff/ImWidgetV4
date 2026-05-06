#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/core/Widget.h>
#include <algorithm>

namespace ImWidgetV4 {

// ==================== ImSlot ====================

ImSlot::ImSlot()
    : m_SlotPosition(0.0f, 0.0f)
    , m_SlotSize(0.0f, 0.0f)
{
}

ImSlot::~ImSlot() {
    // Slot 不持有子控件，无需释放
}

void ImSlot::ApplyLayout(ImWidget* child) {
    if (!child) {
        return;
    }

    // 基础 Slot：子控件占据整个 Slot 空间
    FGeometry geometry;
    geometry.Position = m_SlotPosition;
    geometry.Size = m_SlotSize;
    child->SetGeometry(geometry);
}

// ==================== ImPaddingSlot ====================

ImPaddingSlot::ImPaddingSlot()
    : ImSlot()
    , PaddingLeft(0.0f)
    , PaddingRight(0.0f)
    , PaddingTop(0.0f)
    , PaddingBottom(0.0f)
{
}

void ImPaddingSlot::ApplyLayout(ImWidget* child) {
    if (!child) {
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
    child->SetGeometry(geometry);
}

} // namespace ImWidgetV4
