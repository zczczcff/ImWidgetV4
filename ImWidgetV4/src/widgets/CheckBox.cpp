#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/style/StyleResolvers.h>
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

const Reflection::FTypeDesc& FCheckBoxStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::BackgroundColor>(
            "FCheckBoxStyle",
            "BackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Background color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::HoveredBackgroundColor>(
            "FCheckBoxStyle",
            "HoveredBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Hovered background color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::PressedBackgroundColor>(
            "FCheckBoxStyle",
            "PressedBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Pressed background color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::CheckedBackgroundColor>(
            "FCheckBoxStyle",
            "CheckedBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Checked background color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::DisabledBackgroundColor>(
            "FCheckBoxStyle",
            "DisabledBackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Disabled background color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::BorderColor>(
            "FCheckBoxStyle",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Border color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::CheckMarkColor>(
            "FCheckBoxStyle",
            "CheckMarkColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Check mark color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::TextColor>(
            "FCheckBoxStyle",
            "TextColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Text color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::DisabledTextColor>(
            "FCheckBoxStyle",
            "DisabledTextColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Disabled text color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FColor, &FCheckBoxStyle::FocusedOutlineColor>(
            "FCheckBoxStyle",
            "FocusedOutlineColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Focused outline color"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FMargin, &FCheckBoxStyle::Padding>(
            "FCheckBoxStyle",
            "Padding",
            Reflection::EPropertyKind::Struct,
            "FMargin",
            "Control padding",
            &FMargin::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FCheckBoxStyle, float, &FCheckBoxStyle::IndicatorSize>(
            "FCheckBoxStyle",
            "IndicatorSize",
            Reflection::EPropertyKind::Float,
            "float",
            "Indicator size"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, float, &FCheckBoxStyle::IndicatorCornerRadius>(
            "FCheckBoxStyle",
            "IndicatorCornerRadius",
            Reflection::EPropertyKind::Float,
            "float",
            "Indicator corner radius"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, float, &FCheckBoxStyle::LabelSpacing>(
            "FCheckBoxStyle",
            "LabelSpacing",
            Reflection::EPropertyKind::Float,
            "float",
            "Spacing between indicator and label"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, float, &FCheckBoxStyle::BorderThickness>(
            "FCheckBoxStyle",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Border thickness"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, float, &FCheckBoxStyle::FontSize>(
            "FCheckBoxStyle",
            "FontSize",
            Reflection::EPropertyKind::Float,
            "float",
            "Font size"),
        Reflection::MakeMemberProperty<FCheckBoxStyle, FVector2, &FCheckBoxStyle::MinDesiredSize>(
            "FCheckBoxStyle",
            "MinDesiredSize",
            Reflection::EPropertyKind::Vec2,
            "FVector2",
            "Minimum desired size")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FCheckBoxStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImCheckBox::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImCheckBox, std::string, &ImCheckBox::m_Label>(
            "ImCheckBox",
            "Label",
            Reflection::EPropertyKind::String,
            "std::string",
            "Checkbox label"),
        Reflection::MakeMemberProperty<ImCheckBox, bool, &ImCheckBox::m_bChecked>(
            "ImCheckBox",
            "Checked",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the checkbox is checked"),
        Reflection::MakeMemberProperty<ImCheckBox, bool, &ImCheckBox::m_bDisabled>(
            "ImCheckBox",
            "Disabled",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the checkbox is disabled"),
        Reflection::MakeMemberProperty<ImCheckBox, FCheckBoxStyle, &ImCheckBox::m_Style>(
            "ImCheckBox",
            "Style",
            Reflection::EPropertyKind::Struct,
            "FCheckBoxStyle",
            "Checkbox style",
            &FCheckBoxStyle::StaticTypeDesc())
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImCheckBox",
        &ImWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FCheckBoxStyleReflectionTypeDesc = FCheckBoxStyle::StaticTypeDesc();
const Reflection::FTypeDesc& ImCheckBoxReflectionTypeDesc = ImCheckBox::StaticTypeDesc();

} // namespace

ImCheckBox::ImCheckBox()
    : ImWidget()
{
    SetSupportsKeyboardFocus(true);
    SetHitTestVisible(true);
}

void ImCheckBox::SetLabel(const std::string& label) {
    SetLabel(FText::FromString(label));
}

void ImCheckBox::SetLabel(const FText& label) {
    if (m_LabelText == label) {
        return;
    }

    m_LabelText = label;
    if (label.IsLocalized()) {
        m_Label = label.GetDefaultText().empty() ? label.GetKey() : label.GetDefaultText();
    } else {
        m_Label = label.GetInvariantText();
    }
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

std::string ImCheckBox::GetLabel() const
{
    return ResolveLabel();
}

void ImCheckBox::SetChecked(bool bChecked) {
    if (m_bChecked == bChecked) {
        return;
    }

    m_bChecked = bChecked;
    Invalidate(EInvalidateReason::Paint);
    const std::shared_ptr<ImWidget> keepAlive = weak_from_this().lock();
    (void)keepAlive;
    OnCheckStateChanged.Broadcast(*this, m_bChecked);
}

void ImCheckBox::Toggle() {
    SetChecked(!m_bChecked);
}

void ImCheckBox::SetStyle(const FCheckBoxStyle& style) {
    m_Style = style;
    m_bHasExplicitStyle = true;
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

    const FCheckBoxStyle& style = GetEffectiveStyle();
    const FVector2 labelSize = MeasureLabelSize();
    const float availableHeight =
        std::max(0.0f, m_Geometry.Size.Y - style.Padding.Top - style.Padding.Bottom);
    const float indicatorY =
        m_Geometry.Position.Y + style.Padding.Top +
        std::max(0.0f, (availableHeight - style.IndicatorSize) * 0.5f);
    const FVector2 indicatorMin(
        m_Geometry.Position.X + style.Padding.Left,
        indicatorY);
    const FVector2 indicatorMax = indicatorMin + FVector2(style.IndicatorSize, style.IndicatorSize);

    FColor indicatorBackground = style.BackgroundColor;
    if (m_bDisabled) {
        indicatorBackground = style.DisabledBackgroundColor;
    } else if (m_bPressed) {
        indicatorBackground = style.PressedBackgroundColor;
    } else if (m_bChecked) {
        indicatorBackground = style.CheckedBackgroundColor;
    } else if (m_bHovered) {
        indicatorBackground = style.HoveredBackgroundColor;
    }

    paintContext.DrawContext_.DrawRectFilled(
        indicatorMin,
        indicatorMax,
        indicatorBackground,
        style.IndicatorCornerRadius
    );
    paintContext.DrawContext_.DrawRect(
        indicatorMin,
        indicatorMax,
        HasKeyboardFocus() ? style.FocusedOutlineColor : style.BorderColor,
        style.IndicatorCornerRadius,
        style.BorderThickness
    );

    if (m_bChecked) {
        const float size = style.IndicatorSize;
        const FVector2 checkA(indicatorMin.X + size * 0.22f, indicatorMin.Y + size * 0.55f);
        const FVector2 checkB(indicatorMin.X + size * 0.45f, indicatorMin.Y + size * 0.78f);
        const FVector2 checkC(indicatorMin.X + size * 0.80f, indicatorMin.Y + size * 0.28f);
        paintContext.DrawContext_.DrawLine(checkA, checkB, style.CheckMarkColor, 2.0f);
        paintContext.DrawContext_.DrawLine(checkB, checkC, style.CheckMarkColor, 2.0f);
    }

    const std::string label = ResolveLabel();
    if (!label.empty()) {
        const FVector2 labelPosition(
            indicatorMax.X + style.LabelSpacing,
            m_Geometry.Position.Y + std::max(0.0f, (m_Geometry.Size.Y - labelSize.Y) * 0.5f));

        if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
            paintContext.DrawContext_.GetImDrawList()->AddText(
                ImGui::GetFont(),
                style.FontSize,
                labelPosition.ToImVec2(),
                (m_bDisabled ? style.DisabledTextColor : style.TextColor).ToImU32(),
                label.c_str()
            );
        } else {
            paintContext.DrawContext_.DrawText(
                labelPosition,
                m_bDisabled ? style.DisabledTextColor : style.TextColor,
                label,
                style.FontSize
            );
        }
    }
}

FVector2 ImCheckBox::GetMinSize() const {
    const FCheckBoxStyle& style = GetEffectiveStyle();
    const std::string label = ResolveLabel();
    const FVector2 labelSize = MeasureLabelSize();
    const float contentWidth = style.Padding.Left + style.IndicatorSize +
        (label.empty() ? 0.0f : (style.LabelSpacing + labelSize.X)) +
        style.Padding.Right;
    const float contentHeight = style.Padding.Top +
        std::max(style.IndicatorSize, labelSize.Y) +
        style.Padding.Bottom;

    return FVector2(
        std::max(contentWidth, style.MinDesiredSize.X),
        std::max(contentHeight, style.MinDesiredSize.Y)
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
    return MeasureText(ResolveLabel(), GetEffectiveStyle().FontSize);
}

std::string ImCheckBox::ResolveLabel() const
{
    return m_LabelText.Resolve();
}

const FCheckBoxStyle& ImCheckBox::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveCheckBoxStyle(application->GetStyleSet());
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

void ImCheckBox::PostDeserializeFromJson()
{
    m_LabelText = FText::FromString(m_Label);
}

} // namespace ImWidgetV4
