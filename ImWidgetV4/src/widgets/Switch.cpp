#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <algorithm>

namespace ImWidgetV4 {

const Reflection::FTypeDesc& FSwitchStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::OffTrackColor>(
            "FSwitchStyle",
            "OffTrackColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Off-state track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::OffTrackHoveredColor>(
            "FSwitchStyle",
            "OffTrackHoveredColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Hovered off-state track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::OffTrackPressedColor>(
            "FSwitchStyle",
            "OffTrackPressedColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Pressed off-state track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::OnTrackColor>(
            "FSwitchStyle",
            "OnTrackColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "On-state track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::OnTrackHoveredColor>(
            "FSwitchStyle",
            "OnTrackHoveredColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Hovered on-state track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::OnTrackPressedColor>(
            "FSwitchStyle",
            "OnTrackPressedColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Pressed on-state track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::DisabledTrackColor>(
            "FSwitchStyle",
            "DisabledTrackColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Disabled track color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::ThumbColor>(
            "FSwitchStyle",
            "ThumbColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Thumb color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::ThumbHoveredColor>(
            "FSwitchStyle",
            "ThumbHoveredColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Hovered thumb color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::ThumbPressedColor>(
            "FSwitchStyle",
            "ThumbPressedColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Pressed thumb color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::DisabledThumbColor>(
            "FSwitchStyle",
            "DisabledThumbColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Disabled thumb color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::BorderColor>(
            "FSwitchStyle",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Switch border color"),
        Reflection::MakeMemberProperty<FSwitchStyle, FColor, &FSwitchStyle::FocusedOutlineColor>(
            "FSwitchStyle",
            "FocusedOutlineColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Focused outline color"),
        Reflection::MakeMemberProperty<FSwitchStyle, float, &FSwitchStyle::BorderThickness>(
            "FSwitchStyle",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Switch border thickness"),
        Reflection::MakeMemberProperty<FSwitchStyle, float, &FSwitchStyle::ThumbInset>(
            "FSwitchStyle",
            "ThumbInset",
            Reflection::EPropertyKind::Float,
            "float",
            "Inset between thumb and track"),
        Reflection::MakeMemberProperty<FSwitchStyle, FVector2, &FSwitchStyle::DesiredSize>(
            "FSwitchStyle",
            "DesiredSize",
            Reflection::EPropertyKind::Vec2,
            "FVector2",
            "Desired switch size")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FSwitchStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImSwitch::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImSwitch, bool, &ImSwitch::m_bChecked>(
            "ImSwitch",
            "Checked",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the switch is checked"),
        Reflection::MakeMemberProperty<ImSwitch, bool, &ImSwitch::m_bDisabled>(
            "ImSwitch",
            "Disabled",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the switch is disabled"),
        Reflection::MakeMemberProperty<ImSwitch, FSwitchStyle, &ImSwitch::m_Style>(
            "ImSwitch",
            "Style",
            Reflection::EPropertyKind::Struct,
            "FSwitchStyle",
            "Switch style",
            &FSwitchStyle::StaticTypeDesc())
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImSwitch",
        &ImWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FSwitchStyleReflectionTypeDesc = FSwitchStyle::StaticTypeDesc();
const Reflection::FTypeDesc& ImSwitchReflectionTypeDesc = ImSwitch::StaticTypeDesc();

} // namespace

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
    m_bHasExplicitStyle = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImSwitch::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    const FGeometry trackGeometry = GetTrackGeometry();
    const FSwitchStyle& style = GetEffectiveStyle();
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
        HasKeyboardFocus() ? style.FocusedOutlineColor : style.BorderColor,
        trackRounding,
        style.BorderThickness);

    paintContext.DrawContext_.DrawCircleFilled(
        thumbCenter,
        thumbRadius,
        ResolveThumbColor());
    paintContext.DrawContext_.DrawCircle(
        thumbCenter,
        thumbRadius,
        style.BorderColor,
        0,
        style.BorderThickness);
}

FVector2 ImSwitch::GetMinSize() const
{
    return GetEffectiveStyle().DesiredSize;
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

const FSwitchStyle& ImSwitch::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveSwitchStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

FColor ImSwitch::ResolveTrackColor() const
{
    const FSwitchStyle& style = GetEffectiveStyle();
    if (m_bDisabled) {
        return style.DisabledTrackColor;
    }

    if (m_bChecked) {
        if (m_bPressed) {
            return style.OnTrackPressedColor;
        }
        if (m_bHovered) {
            return style.OnTrackHoveredColor;
        }
        return style.OnTrackColor;
    }

    if (m_bPressed) {
        return style.OffTrackPressedColor;
    }
    if (m_bHovered) {
        return style.OffTrackHoveredColor;
    }
    return style.OffTrackColor;
}

FColor ImSwitch::ResolveThumbColor() const
{
    const FSwitchStyle& style = GetEffectiveStyle();
    if (m_bDisabled) {
        return style.DisabledThumbColor;
    }
    if (m_bPressed) {
        return style.ThumbPressedColor;
    }
    if (m_bHovered) {
        return style.ThumbHoveredColor;
    }
    return style.ThumbColor;
}

FGeometry ImSwitch::GetTrackGeometry() const
{
    const float trackWidth = std::max(0.0f, m_Geometry.Size.X);
    const float trackHeight = std::max(0.0f, m_Geometry.Size.Y);
    return FGeometry(m_Geometry.Position, FVector2(trackWidth, trackHeight));
}

FVector2 ImSwitch::GetThumbCenter() const
{
    const FSwitchStyle& style = GetEffectiveStyle();
    const FGeometry trackGeometry = GetTrackGeometry();
    const float thumbRadius = GetThumbRadius();
    const float minCenterX = trackGeometry.Position.X + style.ThumbInset + thumbRadius;
    const float maxCenterX = trackGeometry.Position.X + trackGeometry.Size.X - style.ThumbInset - thumbRadius;
    const float centerY = trackGeometry.Position.Y + trackGeometry.Size.Y * 0.5f;

    return FVector2(m_bChecked ? maxCenterX : minCenterX, centerY);
}

float ImSwitch::GetThumbRadius() const
{
    const FSwitchStyle& style = GetEffectiveStyle();
    const FGeometry trackGeometry = GetTrackGeometry();
    return std::max(0.0f, (trackGeometry.Size.Y * 0.5f) - style.ThumbInset);
}

} // namespace ImWidgetV4
