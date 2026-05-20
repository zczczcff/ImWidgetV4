#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4 {

struct FSliderStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FSliderStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImSlider"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
    const FSliderStyle& GetStyle() const { return GetEffectiveStyle(); }

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
    const FSliderStyle& GetEffectiveStyle() const;
    void SetHovered(bool bHovered);
    void SetDragging(bool bDragging);
    FVector2 GetTrackMin() const;
    FVector2 GetTrackMax() const;
    float GetThumbCenterX() const;

    FSliderStyle m_Style;
    mutable FSliderStyle m_ResolvedThemeStyle;
    float m_MinValue = 0.0f;
    float m_MaxValue = 1.0f;
    float m_Value = 0.0f;
    float m_Step = 0.0f;
    bool m_bHovered = false;
    bool m_bDragging = false;
    bool m_bDisabled = false;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
