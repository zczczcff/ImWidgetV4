#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>
#include <cmath>
#include <cstdio>

namespace ImWidgetV4 {

namespace {

float Clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

} // namespace

ImSlider::ImSlider()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

void ImSlider::SetValue(float value) {
    SetValueInternal(value, true);
}

void ImSlider::SetRange(float minValue, float maxValue) {
    if (minValue > maxValue) {
        std::swap(minValue, maxValue);
    }

    if (m_MinValue == minValue && m_MaxValue == maxValue) {
        return;
    }

    m_MinValue = minValue;
    m_MaxValue = maxValue;
    SetValueInternal(m_Value, false);
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImSlider::SetStep(float step) {
    const float clampedStep = step < 0.0f ? 0.0f : step;
    if (m_Step == clampedStep) {
        return;
    }

    m_Step = clampedStep;
}

void ImSlider::SetStyle(const FSliderStyle& style) {
    m_Style = style;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImSlider::SetDisabled(bool bDisabled) {
    if (m_bDisabled == bDisabled) {
        return;
    }

    m_bDisabled = bDisabled;
    if (m_bDisabled) {
        m_bDragging = false;
    }
    Invalidate(EInvalidateReason::Paint);
}

void ImSlider::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    const FVector2 trackMin = GetTrackMin();
    const FVector2 trackMax = GetTrackMax();
    const FVector2 filledMax(GetThumbCenterX(), trackMax.Y);
    const float thumbCenterY = (trackMin.Y + trackMax.Y) * 0.5f;
    const FVector2 thumbCenter(GetThumbCenterX(), thumbCenterY);

    const FColor trackColor = m_bDisabled ? m_Style.DisabledTrackColor : m_Style.TrackColor;
    FColor filledColor = m_Style.FilledTrackColor;
    if (m_bDisabled) {
        filledColor = m_Style.DisabledTrackColor;
    } else if (m_bHovered || m_bDragging) {
        filledColor = m_Style.HoveredFilledTrackColor;
    }

    FColor thumbColor = m_Style.ThumbColor;
    if (m_bDisabled) {
        thumbColor = m_Style.DisabledThumbColor;
    } else if (m_bDragging) {
        thumbColor = m_Style.ActiveThumbColor;
    } else if (m_bHovered) {
        thumbColor = m_Style.HoveredThumbColor;
    }

    paintContext.DrawContext_.DrawRectFilled(
        trackMin,
        trackMax,
        trackColor,
        m_Style.TrackRounding
    );

    const FVector2 clampedFilledMax(
        std::max(trackMin.X, filledMax.X),
        trackMax.Y
    );
    paintContext.DrawContext_.DrawRectFilled(
        trackMin,
        clampedFilledMax,
        filledColor,
        m_Style.TrackRounding
    );

    paintContext.DrawContext_.DrawCircleFilled(
        thumbCenter,
        m_Style.ThumbRadius,
        thumbColor
    );

    if (HasKeyboardFocus()) {
        paintContext.DrawContext_.DrawCircle(
            thumbCenter,
            m_Style.ThumbRadius + 3.0f,
            m_Style.FocusedOutlineColor,
            0,
            2.0f
        );
    }

    if (m_Style.bShowValueText) {
        char valueBuffer[32] = {};
        std::snprintf(valueBuffer, sizeof(valueBuffer), "%.2f", m_Value);
        const FVector2 valuePosition(
            trackMax.X + 12.0f,
            m_Geometry.Position.Y + std::max(0.0f, (m_Geometry.Size.Y - m_Style.ValueFontSize) * 0.5f)
        );

        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
            paintContext.DrawContext_.GetImDrawList()->AddText(
                ImGui::GetFont(),
                m_Style.ValueFontSize,
                valuePosition.ToImVec2(),
                m_Style.ValueTextColor.ToImU32(),
                valueBuffer
            );
        } else {
            paintContext.DrawContext_.DrawText(
                valuePosition,
                m_Style.ValueTextColor,
                valueBuffer,
                m_Style.ValueFontSize
            );
        }
    }
}

FVector2 ImSlider::GetMinSize() const {
    return m_Style.MinDesiredSize;
}

FReply ImSlider::OnInputEvent(const FInputEvent& event) {
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
        SetHovered(true);
        SetDragging(true);
        SetValueInternal(ResolveValueFromMouse(event.MousePosition), true);
        return FReply::Handled()
            .SetKeyboardFocus(shared_from_this())
            .CaptureMouse(shared_from_this(), EMouseButton::Left);
    }

    if (event.Type == EInputEventType::MouseMove && m_bDragging) {
        SetValueInternal(ResolveValueFromMouse(event.MousePosition), true);
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_bDragging) {
        SetValueInternal(ResolveValueFromMouse(event.MousePosition), true);
        SetDragging(false);
        return FReply::Handled().ReleaseMouseCapture();
    }

    if (HasKeyboardFocus() && event.Type == EInputEventType::KeyDown) {
        const float step = ResolveKeyboardStep();
        switch (event.Key) {
        case EKey::Left:
            SetValueInternal(m_Value - step, true);
            return FReply::Handled();
        case EKey::Right:
            SetValueInternal(m_Value + step, true);
            return FReply::Handled();
        case EKey::Home:
            SetValueInternal(m_MinValue, true);
            return FReply::Handled();
        case EKey::End:
            SetValueInternal(m_MaxValue, true);
            return FReply::Handled();
        default:
            break;
        }
    }

    return FReply::Unhandled();
}

void ImSlider::OnFocusChanged(bool bHasFocus) {
    ImWidget::OnFocusChanged(bHasFocus);
}

float ImSlider::ClampValue(float value) const {
    if (m_MinValue > m_MaxValue) {
        return value;
    }
    return std::clamp(value, m_MinValue, m_MaxValue);
}

float ImSlider::GetNormalizedValue() const {
    const float range = m_MaxValue - m_MinValue;
    if (range <= 0.0f) {
        return 0.0f;
    }

    return Clamp01((m_Value - m_MinValue) / range);
}

float ImSlider::ResolveValueFromMouse(const FVector2& mousePosition) const {
    const FVector2 trackMin = GetTrackMin();
    const FVector2 trackMax = GetTrackMax();
    const float trackWidth = std::max(1.0f, trackMax.X - trackMin.X);
    const float alpha = Clamp01((mousePosition.X - trackMin.X) / trackWidth);
    return m_MinValue + (m_MaxValue - m_MinValue) * alpha;
}

float ImSlider::ResolveKeyboardStep() const {
    if (m_Step > 0.0f) {
        return m_Step;
    }

    const float range = m_MaxValue - m_MinValue;
    if (range <= 0.0f) {
        return 0.0f;
    }

    return range / 100.0f;
}

void ImSlider::SetValueInternal(float value, bool bBroadcast) {
    const float clampedValue = ClampValue(value);
    if (std::abs(clampedValue - m_Value) <= 0.0001f) {
        return;
    }

    m_Value = clampedValue;
    Invalidate(EInvalidateReason::Paint);

    if (bBroadcast) {
        const std::shared_ptr<ImSlider> keepAlive = std::static_pointer_cast<ImSlider>(shared_from_this());
        OnValueChanged.Broadcast(*this, m_Value);
    }
}

void ImSlider::SetHovered(bool bHovered) {
    if (m_bHovered == bHovered) {
        return;
    }

    m_bHovered = bHovered;
    Invalidate(EInvalidateReason::Paint);
}

void ImSlider::SetDragging(bool bDragging) {
    if (m_bDragging == bDragging) {
        return;
    }

    m_bDragging = bDragging;
    Invalidate(EInvalidateReason::Paint);
}

FVector2 ImSlider::GetTrackMin() const {
    return FVector2(
        m_Geometry.Position.X + m_Style.ThumbRadius,
        m_Geometry.Position.Y + std::max(0.0f, (m_Geometry.Size.Y - m_Style.TrackHeight) * 0.5f)
    );
}

FVector2 ImSlider::GetTrackMax() const {
    const float valueTextWidth = m_Style.bShowValueText ? 52.0f : 0.0f;
    return FVector2(
        m_Geometry.Position.X + m_Geometry.Size.X - m_Style.ThumbRadius - valueTextWidth,
        m_Geometry.Position.Y + std::max(0.0f, (m_Geometry.Size.Y - m_Style.TrackHeight) * 0.5f) + m_Style.TrackHeight
    );
}

float ImSlider::GetThumbCenterX() const {
    const FVector2 trackMin = GetTrackMin();
    const FVector2 trackMax = GetTrackMax();
    return trackMin.X + (trackMax.X - trackMin.X) * GetNormalizedValue();
}

} // namespace ImWidgetV4
