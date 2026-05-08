#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>

namespace ImWidgetV4 {

ImSwitch::ImSwitch()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

void ImSwitch::SetChecked(bool bChecked)
{
    if (m_bChecked == bChecked) {
        return;
    }

    m_bChecked = bChecked;
    Invalidate(EInvalidateReason::Paint);
    const std::shared_ptr<ImWidget> keepAlive = weak_from_this().lock();
    (void)keepAlive;
    OnCheckStateChanged.Broadcast(*this, m_bChecked);
}

void ImSwitch::Toggle()
{
    SetChecked(!m_bChecked);
}

void ImSwitch::SetEnabled(bool bEnabled)
{
    SetDisabled(!bEnabled);
}

void ImSwitch::SetDisabled(bool bDisabled)
{
    if (m_bDisabled == bDisabled) {
        return;
    }

    m_bDisabled = bDisabled;
    if (m_bDisabled) {
        SetPressed(false);
        if (ImApplication* application = GetApplication()) {
            if (application->GetMouseCapture().get() == this) {
                application->ReleaseMouseCapture();
            }
            if (application->GetKeyboardFocus().get() == this) {
                application->ClearKeyboardFocus();
            }
        }
    }
    Invalidate(EInvalidateReason::Paint);
}

void ImSwitch::SetStyle(const FSwitchStyle& style)
{
    m_Style = style;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImSwitch::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    const FGeometry trackGeometry = GetTrackGeometry();
    const float trackRounding = std::max(0.0f, trackGeometry.Size.Y * 0.5f);
    const float thumbRadius = GetThumbRadius();
    const FVector2 thumbCenter = GetThumbCenter();

    paintContext.DrawContext_.DrawRectFilled(
        trackGeometry.GetMin(),
        trackGeometry.GetMax(),
        ResolveTrackColor(),
        trackRounding);
    paintContext.DrawContext_.DrawRect(
        trackGeometry.GetMin(),
        trackGeometry.GetMax(),
        m_Style.BorderColor,
        trackRounding,
        m_Style.BorderThickness);

    if (HasKeyboardFocus()) {
        const FVector2 outlinePadding(1.0f, 1.0f);
        paintContext.DrawContext_.DrawRect(
            m_Geometry.GetMin() + outlinePadding,
            m_Geometry.GetMax() - outlinePadding,
            m_Style.FocusedOutlineColor,
            std::max(0.0f, trackRounding + 2.0f),
            2.0f);
    }

    paintContext.DrawContext_.DrawCircleFilled(
        thumbCenter,
        thumbRadius,
        ResolveThumbColor());
    paintContext.DrawContext_.DrawCircle(
        thumbCenter,
        thumbRadius,
        m_Style.BorderColor,
        0,
        std::max(1.0f, m_Style.BorderThickness));
}

FVector2 ImSwitch::GetMinSize() const
{
    return m_Style.DesiredSize;
}

FReply ImSwitch::OnInputEvent(const FInputEvent& event)
{
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

        if (wasPressed) {
            SetPressed(false);
            OnReleased.Broadcast(*this);
        }

        if (wasPressed && isInside) {
            Toggle();
            return FReply::Handled().ReleaseMouseCapture();
        }

        if (wasPressed) {
            return FReply::Handled().ReleaseMouseCapture();
        }
    }

    if (HasKeyboardFocus() && event.Type == EInputEventType::KeyDown) {
        if (event.Key == EKey::Space || event.Key == EKey::Enter) {
            Toggle();
            return FReply::Handled();
        }
    }

    return FReply::Unhandled();
}

void ImSwitch::OnFocusChanged(bool bHasFocus)
{
    ImWidget::OnFocusChanged(bHasFocus);
}

void ImSwitch::SetHovered(bool bHovered)
{
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

void ImSwitch::SetPressed(bool bPressed)
{
    if (m_bPressed == bPressed) {
        return;
    }

    m_bPressed = bPressed;
    Invalidate(EInvalidateReason::Paint);
}

FColor ImSwitch::ResolveTrackColor() const
{
    if (m_bDisabled) {
        return m_Style.DisabledTrackColor;
    }

    if (m_bChecked) {
        if (m_bPressed) {
            return m_Style.OnTrackPressedColor;
        }
        if (m_bHovered) {
            return m_Style.OnTrackHoveredColor;
        }
        return m_Style.OnTrackColor;
    }

    if (m_bPressed) {
        return m_Style.OffTrackPressedColor;
    }
    if (m_bHovered) {
        return m_Style.OffTrackHoveredColor;
    }
    return m_Style.OffTrackColor;
}

FColor ImSwitch::ResolveThumbColor() const
{
    if (m_bDisabled) {
        return m_Style.DisabledThumbColor;
    }
    if (m_bPressed) {
        return m_Style.ThumbPressedColor;
    }
    if (m_bHovered) {
        return m_Style.ThumbHoveredColor;
    }
    return m_Style.ThumbColor;
}

FGeometry ImSwitch::GetTrackGeometry() const
{
    const float trackWidth = std::max(0.0f, m_Geometry.Size.X);
    const float trackHeight = std::max(0.0f, m_Geometry.Size.Y);
    return FGeometry(m_Geometry.Position, FVector2(trackWidth, trackHeight));
}

FVector2 ImSwitch::GetThumbCenter() const
{
    const FGeometry trackGeometry = GetTrackGeometry();
    const float thumbRadius = GetThumbRadius();
    const float minCenterX = trackGeometry.Position.X + m_Style.ThumbInset + thumbRadius;
    const float maxCenterX = trackGeometry.Position.X + trackGeometry.Size.X - m_Style.ThumbInset - thumbRadius;
    const float centerY = trackGeometry.Position.Y + trackGeometry.Size.Y * 0.5f;

    return FVector2(m_bChecked ? maxCenterX : minCenterX, centerY);
}

float ImSwitch::GetThumbRadius() const
{
    const FGeometry trackGeometry = GetTrackGeometry();
    return std::max(0.0f, (trackGeometry.Size.Y * 0.5f) - m_Style.ThumbInset);
}

} // namespace ImWidgetV4
