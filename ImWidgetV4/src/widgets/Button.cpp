#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <algorithm>

namespace ImWidgetV4 {

namespace {

FGeometry InsetGeometryByBorder(const FGeometry& geometry, float borderThickness)
{
    const float inset = std::max(0.0f, borderThickness);
    return FGeometry(
        FVector2(geometry.Position.X + inset, geometry.Position.Y + inset),
        FVector2(
            std::max(0.0f, geometry.Size.X - inset * 2.0f),
            std::max(0.0f, geometry.Size.Y - inset * 2.0f)));
}

} // namespace

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

    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
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
    SetText(FText::FromString(text));
}

void ImButton::SetText(const FText& text) {
    auto content = GetContent();
    auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(content);

    if (textBlock) {
        textBlock->SetText(text);
        Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
        return;
    }

    auto newTextBlock = std::make_shared<ImTextBlock>();
    newTextBlock->SetText(text);
    newTextBlock->SetTextColor(GetCurrentStateStyle().TextColor);
    SetContent(newTextBlock);
}

void ImButton::SetStyle(const FButtonStyle& style) {
    m_Style = style;
    m_bHasExplicitStyle = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
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
    const FButtonStyle& effectiveStyle = GetEffectiveStyle();
    const auto& children = GetChildren();
    const ImPaddingSlot* slot = dynamic_cast<const ImPaddingSlot*>(GetSlotAt(0));
    const float borderInset = std::max(
        {
            0.0f,
            effectiveStyle.Normal.BorderThickness,
            effectiveStyle.Hovered.BorderThickness,
            effectiveStyle.Pressed.BorderThickness,
            effectiveStyle.Focused.BorderThickness,
            effectiveStyle.Disabled.BorderThickness
        });
    const float totalBorderWidth = borderInset * 2.0f;

    if (!children.empty() && slot) {
        FVector2 contentMinSize = children[0]->GetMinSize();
        contentMinSize.X += slot->PaddingLeft + slot->PaddingRight + totalBorderWidth;
        contentMinSize.Y += slot->PaddingTop + slot->PaddingBottom + totalBorderWidth;

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

        OnPressed.Broadcast(*this);

        return FReply::Handled()
            .SetKeyboardFocus(shared_from_this())
            .CaptureMouse(shared_from_this(), EMouseButton::Left);
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left) {
        const bool wasPressed = m_bPressed;
        const bool isInside = m_Geometry.Contains(event.MousePosition);

        SetPressed(false);
        SetHovered(isInside);

        OnReleased.Broadcast(*this);

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

void ImButton::OnFocusChanged(bool bHasFocus)
{
    ImWidget::OnFocusChanged(bHasFocus);
    if (!bHasFocus && m_bPressed) {
        SetPressed(false);
    }
}

void ImButton::Relayout() {
    ImPaddingSlot* slot = GetContentSlot();
    const auto& children = GetChildren();

    if (slot && !children.empty()) {
        const FGeometry contentGeometry = InsetGeometryByBorder(m_Geometry, GetCurrentStateStyle().BorderThickness);
        slot->SetSlotPosition(contentGeometry.Position);
        slot->SetSlotSize(contentGeometry.Size);
        slot->ApplyLayout(children[0].get());
    }
}

const FButtonStateStyle& ImButton::GetCurrentStateStyle() const {
    const FButtonStyle& effectiveStyle = GetEffectiveStyle();
    if (m_bDisabled) {
        return effectiveStyle.Disabled;
    }
    if (m_bPressed) {
        return effectiveStyle.Pressed;
    }
    if (m_bHovered) {
        return effectiveStyle.Hovered;
    }
    if (HasKeyboardFocus()) {
        return effectiveStyle.Focused;
    }
    return effectiveStyle.Normal;
}

const FButtonStyle& ImButton::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveButtonStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
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
        OnHoverBegin.Broadcast(*this);
    } else {
        OnHoverEnd.Broadcast(*this);
    }
}

void ImButton::TriggerClick() {
    const FButtonStateStyle& currentStyle = GetCurrentStateStyle();
    auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(GetContent());
    if (textBlock) {
        textBlock->SetTextColor(currentStyle.TextColor);
    }
    OnClicked.Broadcast(*this);
}

void ImButton::RenderButton(const FPaintContext& paintContext) {
    const FButtonStateStyle& currentStyle = GetCurrentStateStyle();
    auto textBlock = std::dynamic_pointer_cast<ImTextBlock>(GetContent());
    if (textBlock) {
        textBlock->SetTextColor(currentStyle.TextColor);
    }
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
