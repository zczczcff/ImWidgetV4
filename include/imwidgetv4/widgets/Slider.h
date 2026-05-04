#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4 {

struct FSliderStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FSliderStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "TrackColor", &FSliderStyle::TrackColor, "Slider track color")
        .RegisterProperty(PropertyType::Color, "FilledTrackColor", &FSliderStyle::FilledTrackColor, "Filled track color")
        .RegisterProperty(PropertyType::Color, "HoveredFilledTrackColor", &FSliderStyle::HoveredFilledTrackColor, "Hovered filled track color")
        .RegisterProperty(PropertyType::Color, "DisabledTrackColor", &FSliderStyle::DisabledTrackColor, "Disabled track color")
        .RegisterProperty(PropertyType::Color, "ThumbColor", &FSliderStyle::ThumbColor, "Thumb color")
        .RegisterProperty(PropertyType::Color, "HoveredThumbColor", &FSliderStyle::HoveredThumbColor, "Hovered thumb color")
        .RegisterProperty(PropertyType::Color, "ActiveThumbColor", &FSliderStyle::ActiveThumbColor, "Active thumb color")
        .RegisterProperty(PropertyType::Color, "DisabledThumbColor", &FSliderStyle::DisabledThumbColor, "Disabled thumb color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FSliderStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "ValueTextColor", &FSliderStyle::ValueTextColor, "Value text color")
        .RegisterProperty(PropertyType::Float, "TrackHeight", &FSliderStyle::TrackHeight, "Track height")
        .RegisterProperty(PropertyType::Float, "TrackRounding", &FSliderStyle::TrackRounding, "Track rounding")
        .RegisterProperty(PropertyType::Float, "ThumbRadius", &FSliderStyle::ThumbRadius, "Thumb radius")
        .RegisterProperty(PropertyType::Float, "ValueFontSize", &FSliderStyle::ValueFontSize, "Value font size")
        .RegisterProperty(PropertyType::Bool, "ShowValueText", &FSliderStyle::bShowValueText, "Whether to show the value text")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FSliderStyle::MinDesiredSize, "Minimum desired size");
    END_DECLARE_OBJECT()

public:
    FColor TrackColor = FColor::FromBytes(43, 51, 61);
    FColor FilledTrackColor = FColor::FromBytes(78, 126, 196);
    FColor HoveredFilledTrackColor = FColor::FromBytes(96, 149, 221);
    FColor DisabledTrackColor = FColor::FromBytes(59, 66, 76);
    FColor ThumbColor = FColor::FromBytes(236, 240, 245);
    FColor HoveredThumbColor = FColor::FromBytes(255, 255, 255);
    FColor ActiveThumbColor = FColor::FromBytes(255, 214, 102);
    FColor DisabledThumbColor = FColor::FromBytes(146, 152, 160);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor ValueTextColor = FColor::FromBytes(238, 241, 245);
    float TrackHeight = 6.0f;
    float TrackRounding = 3.0f;
    float ThumbRadius = 8.0f;
    float ValueFontSize = 14.0f;
    bool bShowValueText = true;
    FVector2 MinDesiredSize {180.0f, 32.0f};
};

class ImSlider : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImSlider, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::Float,
            "Value",
            static_cast<void (ImSlider::*)(float&)>(&ImSlider::SetValueProperty),
            static_cast<float& (ImSlider::*)()>(&ImSlider::GetValueProperty),
            "Current slider value")
        .RegisterProperty(
            PropertyType::Float,
            "MinValue",
            static_cast<void (ImSlider::*)(float&)>(&ImSlider::SetMinValueProperty),
            static_cast<float& (ImSlider::*)()>(&ImSlider::GetMinValueProperty),
            "Minimum slider value")
        .RegisterProperty(
            PropertyType::Float,
            "MaxValue",
            static_cast<void (ImSlider::*)(float&)>(&ImSlider::SetMaxValueProperty),
            static_cast<float& (ImSlider::*)()>(&ImSlider::GetMaxValueProperty),
            "Maximum slider value")
        .RegisterProperty(
            PropertyType::Float,
            "Step",
            static_cast<void (ImSlider::*)(float&)>(&ImSlider::SetStepProperty),
            static_cast<float& (ImSlider::*)()>(&ImSlider::GetStepProperty),
            "Slider step value")
        .RegisterProperty(PropertyType::Bool, "Disabled", &ImSlider::m_bDisabled, "Whether the slider is disabled")
        .RegisterProperty(PropertyType::Struct, "Style", &ImSlider::m_Style, "Slider style");
    END_DECLARE_OBJECT()

public:
    using FSliderValueChangedEvent = TMulticastDelegate<ImSlider&, float>;

    ImSlider();
    virtual ~ImSlider() = default;

    void SetValue(float value);
    float GetValue() const { return m_Value; }

    void SetRange(float minValue, float maxValue);
    float GetMinValue() const { return m_MinValue; }
    float GetMaxValue() const { return m_MaxValue; }

    void SetStep(float step);
    float GetStep() const { return m_Step; }

    void SetStyle(const FSliderStyle& style);
    const FSliderStyle& GetStyle() const { return m_Style; }

    void SetDisabled(bool bDisabled);
    bool IsDisabled() const { return m_bDisabled; }
    bool IsHovered() const { return m_bHovered; }
    bool IsDragging() const { return m_bDragging; }

    FSliderValueChangedEvent OnValueChanged;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    void SetValueProperty(float& value) { SetValue(value); }
    float& GetValueProperty() { return m_Value; }
    void SetMinValueProperty(float& value) { SetRange(value, m_MaxValue); }
    float& GetMinValueProperty() { return m_MinValue; }
    void SetMaxValueProperty(float& value) { SetRange(m_MinValue, value); }
    float& GetMaxValueProperty() { return m_MaxValue; }
    void SetStepProperty(float& value) { SetStep(value); }
    float& GetStepProperty() { return m_Step; }

    float ClampValue(float value) const;
    float GetNormalizedValue() const;
    float ResolveValueFromMouse(const FVector2& mousePosition) const;
    float ResolveKeyboardStep() const;
    void SetValueInternal(float value, bool bBroadcast);
    void SetHovered(bool bHovered);
    void SetDragging(bool bDragging);
    FVector2 GetTrackMin() const;
    FVector2 GetTrackMax() const;
    float GetThumbCenterX() const;

    FSliderStyle m_Style;
    float m_MinValue = 0.0f;
    float m_MaxValue = 1.0f;
    float m_Value = 0.0f;
    float m_Step = 0.0f;
    bool m_bHovered = false;
    bool m_bDragging = false;
    bool m_bDisabled = false;
};

} // namespace ImWidgetV4
