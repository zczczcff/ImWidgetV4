#include <imwidgetv4/core/Widget.h>
#include <algorithm>

namespace ImWidgetV4 {

ImWidget::ImWidget()
    : m_Name("")
    , m_bVisible(true)
    , m_bHitTestVisible(true)
    , m_bSupportsKeyboardFocus(false)
    , m_bHasKeyboardFocus(false)
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

// ==================== 事件处理 ====================

FReply ImWidget::OnPreviewInputEvent(const FInputEvent& event) {
    // 基类默认不处理预览事件
    // 子类可以重写此方法以在事件到达目标控件之前拦截事件
    return FReply::Unhandled();
}

FReply ImWidget::OnInputEvent(const FInputEvent& event) {
    // 基类默认不处理事件
    // 子类应重写此方法以实现自定义事件处理逻辑
    return FReply::Unhandled();
}

bool ImWidget::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) {
    // 检查控件是否参与命中测试且包含该位置
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    // 将自己添加到路径
    outPath.push_back(shared_from_this());

    // 从后向前测试子控件（从上到下）
    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        if ((*it)->BuildHitTestPath(position, outPath)) {
            return true;  // 子控件处理了
        }
    }

    return true;  // 此控件处理
}

// ==================== 子控件管理 ====================

void ImWidget::AddChild(const Ptr& child) {
    if (!child) {
        return;
    }

    // 设置父指针（弱引用）
    child->m_Parent = shared_from_this();

    // 添加到子控件列表
    m_Children.push_back(child);
}

void ImWidget::ClearChildren() {
    if (m_Children.empty()) {
        return;
    }

    // 清除所有子控件的父指针
    for (const Ptr& child : m_Children) {
        if (child) {
            child->m_Parent.reset();
        }
    }

    // 清空子控件列表
    m_Children.clear();
}

std::shared_ptr<ImWidget> ImWidget::GetParent() const {
    return m_Parent.lock();
}

} // namespace ImWidgetV4
