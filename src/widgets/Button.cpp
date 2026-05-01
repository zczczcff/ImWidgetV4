#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>

namespace ImWidgetV4 {

ImButton::ImButton()
    : ImPanelWidget()
    , m_Style()
    , m_bHovered(false)
    , m_bPressed(false)
    , m_bDisabled(false)
    , m_OriginalMinSize(100.0f, 30.0f)
{
    // 按钮默认支持键盘焦点
    SetSupportsKeyboardFocus(true);

    // 按钮默认参与命中测试
    SetHitTestVisible(true);
}

// ==================== 内容管理 ====================

void ImButton::SetContent(const Ptr& child) {
    // 清除所有子控件
    ClearChildren();

    // 添加新的子控件和 Slot
    if (child) {
        AddSlot(child);
    }
}

ImButton::Ptr ImButton::GetContent() {
    const auto& children = GetChildren();
    if (!children.empty()) {
        return children[0];
    }
    return nullptr;
}

ImPaddingSlot* ImButton::GetContentSlot() {
    return dynamic_cast<ImPaddingSlot*>(GetSlotAt(0));
}

// ==================== 便捷方法（向后兼容） ====================

void ImButton::SetText(const std::string& text) {
    // 检查当前内容是否是 ImTextBlock
    auto content = GetContent();
    auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(content);

    if (textBlock) {
        // 如果已经是 ImTextBlock，直接设置文本
        textBlock->SetText(text);
    } else {
        // 否则创建新的 ImTextBlock
        auto newTextBlock = std::make_shared<ImTextBlock>();
        newTextBlock->SetText(text);
        newTextBlock->SetTextColor(FColor::Black);
        SetContent(newTextBlock);
    }
}

std::string ImButton::GetText() const {
    // 检查当前内容是否是 ImTextBlock
    const auto& children = GetChildren();
    if (!children.empty()) {
        auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(children[0]);
        if (textBlock) {
            return textBlock->GetText();
        }
    }

    return "";
}

// ==================== 重写基类方法 ====================

std::unique_ptr<ImSlot> ImButton::CreateSlot() {
    // 创建带内边距的 Slot
    auto slot = std::make_unique<ImPaddingSlot>();

    // 设置默认内边距
    slot->PaddingLeft = 10.0f;
    slot->PaddingRight = 10.0f;
    slot->PaddingTop = 5.0f;
    slot->PaddingBottom = 5.0f;

    return slot;
}

void ImButton::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    // 1. 实时悬停检测（基于 paintContext 的光标位置）
    if (paintContext.bHasCursorPosition) {
        bool isHoveredNow = m_Geometry.Contains(paintContext.CursorPosition);

        // 如果实时检测到悬停状态变化，更新状态并触发回调
        if (isHoveredNow != m_bHovered) {
            bool wasHovered = m_bHovered;
            m_bHovered = isHoveredNow;

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
        }
    }

    // 2. 重新布局（如果需要）
    Relayout();

    // 3. 绘制按钮背景和边框
    RenderButton(paintContext);

    // 4. 绘制子控件（内容）
    RenderChildren(paintContext);
}

FVector2 ImButton::GetMinSize() const {
    // 获取子控件的最小尺寸
    const auto& children = GetChildren();
    const ImPaddingSlot* slot = dynamic_cast<const ImPaddingSlot*>(GetSlotAt(0));

    if (!children.empty() && slot) {
        FVector2 contentMinSize = children[0]->GetMinSize();

        // 加上内边距
        contentMinSize.X += slot->PaddingLeft + slot->PaddingRight;
        contentMinSize.Y += slot->PaddingTop + slot->PaddingBottom;

        // 返回内容尺寸和原始最小尺寸的最大值
        return FVector2(
            std::max(contentMinSize.X, m_OriginalMinSize.X),
            std::max(contentMinSize.Y, m_OriginalMinSize.Y)
        );
    }

    return m_OriginalMinSize;
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

        SetPressed(true);

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

        SetPressed(false);

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

void ImButton::Relayout() {
    ImPaddingSlot* slot = GetContentSlot();
    const auto& children = GetChildren();

    if (slot && !children.empty()) {
        // 设置 Slot 的位置和大小为按钮的几何信息
        slot->SetSlotPosition(m_Geometry.Position);
        slot->SetSlotSize(m_Geometry.Size);

        // 应用布局（计算子控件的实际位置和大小）
        slot->ApplyLayout(children[0].get());
    }
}

// ==================== 内部方法 ====================

const FButtonStateStyle& ImButton::GetCurrentStateStyle() const {
    // 状态优先级：Disabled > Pressed > Hovered > Focused > Normal
    if (m_bDisabled) {
        return m_Style.Disabled;
    }
    if (m_bPressed) {
        return m_Style.Pressed;
    }
    if (m_bHovered) {
        return m_Style.Hovered;
    }
    if (HasKeyboardFocus()) {
        return m_Style.Focused;
    }
    return m_Style.Normal;
}

void ImButton::SetPressed(bool bPressed) {
    if (m_bPressed == bPressed) {
        return;
    }

    m_bPressed = bPressed;
    Invalidate(EInvalidateReason::Paint);
}

void ImButton::TriggerClick() {
    if (m_OnClicked) {
        m_OnClicked();
    }
}

void ImButton::RenderButton(const FPaintContext& paintContext) {
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
}

} // namespace ImWidgetV4
