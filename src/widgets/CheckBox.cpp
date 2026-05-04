#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

FVector2 MeasureText(const std::string& text, float fontSize) {
    if (text.empty()) {
        return FVector2(0.0f, fontSize);
    }

    if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr) {
        return FVector2(static_cast<float>(text.size()) * fontSize * 0.55f, fontSize);
    }

    ImFont* font = ImGui::GetFont();
    const ImVec2 size = font->CalcTextSizeA(
        fontSize,
        FLT_MAX,
        0.0f,
        text.c_str(),
        text.c_str() + text.size());
    return FVector2(size.x, size.y);
}

} // namespace

ImCheckBox::ImCheckBox()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

void ImCheckBox::SetLabel(const std::string& label) {
    if (m_Label == label) {
        return;
    }

    m_Label = label;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImCheckBox::SetChecked(bool bChecked) {
    if (m_bChecked == bChecked) {
        return;
    }

    m_bChecked = bChecked;
    Invalidate(EInvalidateReason::Paint);
    OnCheckStateChanged.Broadcast(*this, m_bChecked);
}

void ImCheckBox::Toggle() {
    SetChecked(!m_bChecked);
}

void ImCheckBox::SetStyle(const FCheckBoxStyle& style) {
    m_Style = style;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImCheckBox::SetDisabled(bool bDisabled) {
    if (m_bDisabled == bDisabled) {
        return;
    }

    m_bDisabled = bDisabled;
    Invalidate(EInvalidateReason::Paint);
}

void ImCheckBox::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    const FVector2 labelSize = MeasureLabelSize();
    const float availableHeight =
        std::max(0.0f, m_Geometry.Size.Y - m_Style.Padding.Top - m_Style.Padding.Bottom);
    const float indicatorY =
        m_Geometry.Position.Y + m_Style.Padding.Top +
        std::max(0.0f, (availableHeight - m_Style.IndicatorSize) * 0.5f);
    const FVector2 indicatorMin(
        m_Geometry.Position.X + m_Style.Padding.Left,
        indicatorY);
    const FVector2 indicatorMax = indicatorMin + FVector2(m_Style.IndicatorSize, m_Style.IndicatorSize);

    FColor indicatorBackground = m_Style.BackgroundColor;
    if (m_bDisabled) {
        indicatorBackground = m_Style.DisabledBackgroundColor;
    } else if (m_bPressed) {
        indicatorBackground = m_Style.PressedBackgroundColor;
    } else if (m_bChecked) {
        indicatorBackground = m_Style.CheckedBackgroundColor;
    } else if (m_bHovered) {
        indicatorBackground = m_Style.HoveredBackgroundColor;
    }

    paintContext.DrawContext_.DrawRectFilled(
        indicatorMin,
        indicatorMax,
        indicatorBackground,
        m_Style.IndicatorCornerRadius
    );
    paintContext.DrawContext_.DrawRect(
        indicatorMin,
        indicatorMax,
        m_Style.BorderColor,
        m_Style.IndicatorCornerRadius,
        m_Style.BorderThickness
    );

    if (HasKeyboardFocus()) {
        const FVector2 outlineMin = m_Geometry.Position + FVector2(1.0f, 1.0f);
        const FVector2 outlineMax = m_Geometry.Position + m_Geometry.Size - FVector2(1.0f, 1.0f);
        paintContext.DrawContext_.DrawRect(
            outlineMin,
            outlineMax,
            m_Style.FocusedOutlineColor,
            m_Style.IndicatorCornerRadius + 2.0f,
            2.0f
        );
    }

    if (m_bChecked) {
        const float size = m_Style.IndicatorSize;
        const FVector2 checkA(indicatorMin.X + size * 0.22f, indicatorMin.Y + size * 0.55f);
        const FVector2 checkB(indicatorMin.X + size * 0.45f, indicatorMin.Y + size * 0.78f);
        const FVector2 checkC(indicatorMin.X + size * 0.80f, indicatorMin.Y + size * 0.28f);
        paintContext.DrawContext_.DrawLine(checkA, checkB, m_Style.CheckMarkColor, 2.0f);
        paintContext.DrawContext_.DrawLine(checkB, checkC, m_Style.CheckMarkColor, 2.0f);
    }

    if (!m_Label.empty()) {
        const FVector2 labelPosition(
            indicatorMax.X + m_Style.LabelSpacing,
            m_Geometry.Position.Y + std::max(0.0f, (m_Geometry.Size.Y - labelSize.Y) * 0.5f));

        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
            paintContext.DrawContext_.GetImDrawList()->AddText(
                ImGui::GetFont(),
                m_Style.FontSize,
                labelPosition.ToImVec2(),
                (m_bDisabled ? m_Style.DisabledTextColor : m_Style.TextColor).ToImU32(),
                m_Label.c_str()
            );
        } else {
            paintContext.DrawContext_.DrawText(
                labelPosition,
                m_bDisabled ? m_Style.DisabledTextColor : m_Style.TextColor,
                m_Label,
                m_Style.FontSize
            );
        }
    }
}

FVector2 ImCheckBox::GetMinSize() const {
    const FVector2 labelSize = MeasureLabelSize();
    const float contentWidth = m_Style.Padding.Left + m_Style.IndicatorSize +
        (m_Label.empty() ? 0.0f : (m_Style.LabelSpacing + labelSize.X)) +
        m_Style.Padding.Right;
    const float contentHeight = m_Style.Padding.Top +
        std::max(m_Style.IndicatorSize, labelSize.Y) +
        m_Style.Padding.Bottom;

    return FVector2(
        std::max(contentWidth, m_Style.MinDesiredSize.X),
        std::max(contentHeight, m_Style.MinDesiredSize.Y)
    );
}

FReply ImCheckBox::OnInputEvent(const FInputEvent& event) {
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
        if (event.Key == EKey::Enter || event.Key == EKey::Space) {
            Toggle();
            return FReply::Handled();
        }
    }

    return FReply::Unhandled();
}

void ImCheckBox::OnFocusChanged(bool bHasFocus) {
    ImWidget::OnFocusChanged(bHasFocus);
}

void ImCheckBox::SetPressed(bool bPressed) {
    if (m_bPressed == bPressed) {
        return;
    }

    m_bPressed = bPressed;
    Invalidate(EInvalidateReason::Paint);
}

void ImCheckBox::SetHovered(bool bHovered) {
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

FVector2 ImCheckBox::MeasureLabelSize() const {
    return MeasureText(m_Label, m_Style.FontSize);
}

} // namespace ImWidgetV4
