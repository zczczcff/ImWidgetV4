#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <algorithm>

namespace ImWidgetV4 {

ImButton::ImButton()
    : ImPanelWidget()
    , m_Style()
    , m_bHovered(false)
    , m_bPressed(false)
    , m_bDisabled(false)
    , m_OriginalMinSize(100.0f, 30.0f) {
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

void ImButton::SetContent(const Ptr& child) {
    ClearChildren();

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

void ImButton::SetText(const std::string& text) {
    auto content = GetContent();
    auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(content);

    if (textBlock) {
        textBlock->SetText(text);
        return;
    }

    auto newTextBlock = std::make_shared<ImTextBlock>();
    newTextBlock->SetText(text);
    newTextBlock->SetTextColor(FColor::Black);
    SetContent(newTextBlock);
}

std::string ImButton::GetText() const {
    const auto& children = GetChildren();
    if (!children.empty()) {
        auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(children[0]);
        if (textBlock) {
            return textBlock->GetText();
        }
    }

    return "";
}

std::unique_ptr<ImSlot> ImButton::CreateSlot() {
    auto slot = std::make_unique<ImPaddingSlot>();
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

    Relayout();
    RenderButton(paintContext);
    RenderChildren(paintContext);
}

FVector2 ImButton::GetMinSize() const {
    const auto& children = GetChildren();
    const ImPaddingSlot* slot = dynamic_cast<const ImPaddingSlot*>(GetSlotAt(0));

    if (!children.empty() && slot) {
        FVector2 contentMinSize = children[0]->GetMinSize();
        contentMinSize.X += slot->PaddingLeft + slot->PaddingRight;
        contentMinSize.Y += slot->PaddingTop + slot->PaddingBottom;

        return FVector2(
            std::max(contentMinSize.X, m_OriginalMinSize.X),
            std::max(contentMinSize.Y, m_OriginalMinSize.Y)
        );
    }

    return m_OriginalMinSize;
}

FReply ImButton::OnInputEvent(const FInputEvent& event) {
    if (m_bDisabled) {
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseEnter) {
        SetHovered(true);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        SetHovered(false);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {
        SetPressed(true);

        if (m_OnPressed) {
            m_OnPressed();
        }

        return FReply::Handled()
            .SetKeyboardFocus(shared_from_this())
            .CaptureMouse(shared_from_this(), EMouseButton::Left);
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left) {
        const bool wasPressed = m_bPressed;
        const bool isInside = m_Geometry.Contains(event.MousePosition);

        SetPressed(false);

        if (m_OnReleased) {
            m_OnReleased();
        }

        if (wasPressed && isInside) {
            TriggerClick();
            return FReply::Handled().ReleaseMouseCapture();
        }

        if (wasPressed) {
            return FReply::Handled().ReleaseMouseCapture();
        }
    }

    if (HasKeyboardFocus() && event.Type == EInputEventType::KeyDown) {
        if (event.Key == EKey::Enter || event.Key == EKey::Space) {
            TriggerClick();
            return FReply::Handled();
        }
    }

    return FReply::Unhandled();
}

void ImButton::Relayout() {
    ImPaddingSlot* slot = GetContentSlot();
    const auto& children = GetChildren();

    if (slot && !children.empty()) {
        slot->SetSlotPosition(m_Geometry.Position);
        slot->SetSlotSize(m_Geometry.Size);
        slot->ApplyLayout(children[0].get());
    }
}

const FButtonStateStyle& ImButton::GetCurrentStateStyle() const {
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

void ImButton::SetHovered(bool bHovered) {
    if (m_bHovered == bHovered) {
        return;
    }

    m_bHovered = bHovered;
    Invalidate(EInvalidateReason::Paint);

    if (m_bHovered) {
        if (m_OnHoverBegin) {
            m_OnHoverBegin();
        }
    } else if (m_OnHoverEnd) {
        m_OnHoverEnd();
    }
}

void ImButton::TriggerClick() {
    if (m_OnClicked) {
        m_OnClicked();
    }
}

void ImButton::RenderButton(const FPaintContext& paintContext) {
    const FButtonStateStyle& currentStyle = GetCurrentStateStyle();
    const FGeometry& geometry = GetGeometry();
    const FVector2 min = geometry.Position;
    const FVector2 max = geometry.Position + geometry.Size;

    paintContext.DrawContext_.DrawRectFilled(
        min,
        max,
        currentStyle.BackgroundColor,
        currentStyle.CornerRadius
    );

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
