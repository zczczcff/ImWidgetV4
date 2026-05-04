#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <string>

namespace ImWidgetV4 {

struct FCheckBoxStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FCheckBoxStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FCheckBoxStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "HoveredBackgroundColor", &FCheckBoxStyle::HoveredBackgroundColor, "Hovered background color")
        .RegisterProperty(PropertyType::Color, "PressedBackgroundColor", &FCheckBoxStyle::PressedBackgroundColor, "Pressed background color")
        .RegisterProperty(PropertyType::Color, "CheckedBackgroundColor", &FCheckBoxStyle::CheckedBackgroundColor, "Checked background color")
        .RegisterProperty(PropertyType::Color, "DisabledBackgroundColor", &FCheckBoxStyle::DisabledBackgroundColor, "Disabled background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FCheckBoxStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "CheckMarkColor", &FCheckBoxStyle::CheckMarkColor, "Check mark color")
        .RegisterProperty(PropertyType::Color, "TextColor", &FCheckBoxStyle::TextColor, "Text color")
        .RegisterProperty(PropertyType::Color, "DisabledTextColor", &FCheckBoxStyle::DisabledTextColor, "Disabled text color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FCheckBoxStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FCheckBoxStyle::Padding, "Control padding")
        .RegisterProperty(PropertyType::Float, "IndicatorSize", &FCheckBoxStyle::IndicatorSize, "Indicator size")
        .RegisterProperty(PropertyType::Float, "IndicatorCornerRadius", &FCheckBoxStyle::IndicatorCornerRadius, "Indicator corner radius")
        .RegisterProperty(PropertyType::Float, "LabelSpacing", &FCheckBoxStyle::LabelSpacing, "Spacing between indicator and label")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FCheckBoxStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "FontSize", &FCheckBoxStyle::FontSize, "Font size")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FCheckBoxStyle::MinDesiredSize, "Minimum desired size");
    END_DECLARE_OBJECT()

public:
    FColor BackgroundColor = FColor::FromBytes(31, 37, 46);
    FColor HoveredBackgroundColor = FColor::FromBytes(42, 51, 62);
    FColor PressedBackgroundColor = FColor::FromBytes(23, 29, 37);
    FColor CheckedBackgroundColor = FColor::FromBytes(78, 126, 196);
    FColor DisabledBackgroundColor = FColor::FromBytes(56, 60, 66);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor CheckMarkColor = FColor::FromBytes(248, 250, 252);
    FColor TextColor = FColor::FromBytes(238, 241, 245);
    FColor DisabledTextColor = FColor::FromBytes(140, 147, 156);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FMargin Padding = FMargin(6.0f);
    float IndicatorSize = 18.0f;
    float IndicatorCornerRadius = 4.0f;
    float LabelSpacing = 10.0f;
    float BorderThickness = 1.0f;
    float FontSize = 16.0f;
    FVector2 MinDesiredSize {148.0f, 32.0f};
};

class ImCheckBox : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImCheckBox, ImWidget)
    registrar
        .RegisterProperty(PropertyType::String, "Label", &ImCheckBox::m_Label, "Checkbox label")
        .RegisterProperty(PropertyType::Bool, "Checked", &ImCheckBox::m_bChecked, "Whether the checkbox is checked")
        .RegisterProperty(PropertyType::Bool, "Disabled", &ImCheckBox::m_bDisabled, "Whether the checkbox is disabled")
        .RegisterProperty(PropertyType::Struct, "Style", &ImCheckBox::m_Style, "Checkbox style");
    END_DECLARE_OBJECT()

public:
    using FCheckBoxEvent = TMulticastDelegate<ImCheckBox&>;
    using FCheckStateChangedEvent = TMulticastDelegate<ImCheckBox&, bool>;

    ImCheckBox();
    virtual ~ImCheckBox() = default;

    void SetLabel(const std::string& label);
    const std::string& GetLabel() const { return m_Label; }

    void SetChecked(bool bChecked);
    bool IsChecked() const { return m_bChecked; }
    void Toggle();

    void SetStyle(const FCheckBoxStyle& style);
    const FCheckBoxStyle& GetStyle() const { return m_Style; }

    void SetDisabled(bool bDisabled);
    bool IsDisabled() const { return m_bDisabled; }
    bool IsHovered() const { return m_bHovered; }
    bool IsPressed() const { return m_bPressed; }

    FCheckStateChangedEvent OnCheckStateChanged;
    FCheckBoxEvent OnPressed;
    FCheckBoxEvent OnReleased;
    FCheckBoxEvent OnHoverBegin;
    FCheckBoxEvent OnHoverEnd;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    void SetPressed(bool bPressed);
    void SetHovered(bool bHovered);
    FVector2 MeasureLabelSize() const;

    FCheckBoxStyle m_Style;
    std::string m_Label;
    bool m_bChecked = false;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;
};

} // namespace ImWidgetV4
