#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4 {

ImWidget::ImWidget()
    : m_Name("")
    , m_bVisible(true)
    , m_Geometry()
{
}

void ImWidget::Paint(const FPaintContext& paintContext) {
    // 基类默认不绘制任何内容
    // 子类应重写此方法以实现自定义绘制逻辑
}

FVector2 ImWidget::GetMinSize() const {
    // 基类默认返回零尺寸
    // 子类应重写此方法以返回实际所需的最小尺寸
    return FVector2(0.0f, 0.0f);
}

} // namespace ImWidgetV4
