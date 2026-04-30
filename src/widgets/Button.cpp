#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>

namespace ImWidgetV4 {

ImButton::ImButton()
    : ImWidget()
    , m_Text("")
    , m_Style()
    , m_bHovered(false)
    , m_bPressed(false)
    , m_bDisabled(false)
    , m_MinSize(100.0f, 30.0f)
{
    // 按钮默认支持键盘焦点
    SetSupportsKeyboardFocus(true);

    // 按钮默认参与命中测试
    SetHitTestVisible(true);
}

// ==================== 重写基类方法 ====================

void ImButton::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    // 获取当前状态的样式
    const FButtonStateStyle& currentStyle = GetCurrentStateStyle();

    // 获取按钮的几何信息
    const FGeometry& geometry = GetGeometry();
    FVector2 min = geometry.Position;
    FVector2 max = geometry.Position + geometry.Size;

    // 绘制背景
    paintContext.DrawContext_.DrawRectFilled(
        min,
        max,
        currentStyle.BackgroundColor,
        currentStyle.CornerRadius
    );

    // 绘制边框（如果启用）
    if (currentStyle.bHasBorder) {
        paintContext.DrawContext_.DrawRect(
            min,
            max,
            currentStyle.BorderColor,
            currentStyle.CornerRadius,
            currentStyle.BorderThickness
        );
    }

    // 绘制文本（如果有）
    if (!m_Text.empty() && ImGui::GetCurrentContext() != nullptr) {
        // 计算文本尺寸
        ImVec2 textSize = ImGui::CalcTextSize(m_Text.c_str());

        // 计算文本居中位置
        FVector2 textPos(
            geometry.Position.X + (geometry.Size.X - textSize.x) * 0.5f,
            geometry.Position.Y + (geometry.Size.Y - textSize.y) * 0.5f
        );

        // 绘制文本
        paintContext.DrawContext_.DrawText(
            textPos,
            currentStyle.TextColor,
            m_Text,
            0.0f  // 使用默认字体大小
        );
    }

    // 绘制子控件（如果有）
    for (const auto& child : m_Children) {
        if (child && child->IsVisible()) {
            child->Paint(paintContext);
        }
    }
}

FVector2 ImButton::GetMinSize() const {
    // 如果有文本，计算文本尺寸
    if (!m_Text.empty() && ImGui::GetCurrentContext() != nullptr) {
        ImVec2 textSize = ImGui::CalcTextSize(m_Text.c_str());

        // 添加内边距
        const float paddingX = 20.0f;
        const float paddingY = 10.0f;

        FVector2 textMinSize(
            textSize.x + paddingX * 2.0f,
            textSize.y + paddingY * 2.0f
        );

        // 返回文本尺寸和最小尺寸的最大值
        return FVector2(
            std::max(textMinSize.X, m_MinSize.X),
            std::max(textMinSize.Y, m_MinSize.Y)
        );
    }

    return m_MinSize;
}

FReply ImButton::OnInputEvent(const FInputEvent& event) {
    // 如果禁用，不处理事件
    if (m_bDisabled) {
        return FReply::Unhandled();
    }

    // 处理鼠标移动事件（更新悬停状态）
    if (event.Type == EInputEventType::MouseMove) {
        bool wasHovered = m_bHovered;
        m_bHovered = m_Geometry.Contains(event.MousePosition);

        // 触发悬停开始/结束回调
        if (m_bHovered && !wasHovered) {
            if (m_OnHoverBegin) {
                m_OnHoverBegin();
            }
        } else if (!m_bHovered && wasHovered) {
            if (m_OnHoverEnd) {
                m_OnHoverEnd();
            }
        }

        return FReply::Unhandled();  // 不消费移动事件
    }

    // 处理鼠标按下事件
    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {

        m_bPressed = true;

        // 触发按下回调
        if (m_OnPressed) {
            m_OnPressed();
        }

        return FReply::Handled();  // 消费事件
    }

    // 处理鼠标释放事件
    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left) {

        bool wasPressed = m_bPressed;
        bool isInside = m_Geometry.Contains(event.MousePosition);

        m_bPressed = false;

        // 触发释放回调
        if (m_OnReleased) {
            m_OnReleased();
        }

        // 如果在按钮内释放，触发点击
        if (wasPressed && isInside) {
            TriggerClick();
            return FReply::Handled();  // 消费事件
        }
    }

    // 处理键盘事件（Enter 或 Space 键）
    if (HasKeyboardFocus() && event.Type == EInputEventType::KeyDown) {
        if (event.Key == ImGuiKey_Enter || event.Key == ImGuiKey_Space) {
            TriggerClick();
            return FReply::Handled();  // 消费事件
        }
    }

    return FReply::Unhandled();
}

// ==================== 内部方法 ====================

const FButtonStateStyle& ImButton::GetCurrentStateStyle() const {
    // 状态优先级：Disabled > Pressed > Hovered > Normal
    if (m_bDisabled) {
        return m_Style.Disabled;
    }
    if (m_bPressed) {
        return m_Style.Pressed;
    }
    if (m_bHovered) {
        return m_Style.Hovered;
    }
    return m_Style.Normal;
}

void ImButton::TriggerClick() {
    if (m_OnClicked) {
        m_OnClicked();
    }
}

} // namespace ImWidgetV4
