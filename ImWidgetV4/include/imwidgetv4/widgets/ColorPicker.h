#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4 {

struct FColorPickerStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FColorPickerStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FColorPickerStyle::BackgroundColor, "Picker background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FColorPickerStyle::BorderColor, "Picker border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FColorPickerStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "CheckerLightColor", &FColorPickerStyle::CheckerLightColor, "Checkerboard light color")
        .RegisterProperty(PropertyType::Color, "CheckerDarkColor", &FColorPickerStyle::CheckerDarkColor, "Checkerboard dark color")
        .RegisterProperty(PropertyType::Color, "SelectorOuterColor", &FColorPickerStyle::SelectorOuterColor, "Selector outer color")
        .RegisterProperty(PropertyType::Color, "SelectorInnerColor", &FColorPickerStyle::SelectorInnerColor, "Selector inner color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FColorPickerStyle::Padding, "Color picker padding")
        .RegisterProperty(PropertyType::Float, "HueBarWidth", &FColorPickerStyle::HueBarWidth, "Hue bar width")
        .RegisterProperty(PropertyType::Float, "AlphaBarWidth", &FColorPickerStyle::AlphaBarWidth, "Alpha bar width")
        .RegisterProperty(PropertyType::Float, "BarSpacing", &FColorPickerStyle::BarSpacing, "Spacing between panels")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FColorPickerStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FColorPickerStyle::CornerRadius, "Outer corner radius")
        .RegisterProperty(PropertyType::Float, "SelectorRadius", &FColorPickerStyle::SelectorRadius, "Selector radius")
        .RegisterProperty(PropertyType::Bool, "ShowAlphaBar", &FColorPickerStyle::bShowAlphaBar, "Whether alpha bar is visible")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FColorPickerStyle::MinDesiredSize, "Minimum desired size");
    END_DECLARE_OBJECT()

public:
    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor CheckerLightColor = FColor::FromBytes(180, 186, 194);
    FColor CheckerDarkColor = FColor::FromBytes(118, 126, 136);
    FColor SelectorOuterColor = FColor::White;
    FColor SelectorInnerColor = FColor::Black;
    FMargin Padding = FMargin(8.0f);
    float HueBarWidth = 18.0f;
    float AlphaBarWidth = 18.0f;
    float BarSpacing = 8.0f;
    float BorderThickness = 1.0f;
    float CornerRadius = 6.0f;
    float SelectorRadius = 6.0f;
    bool bShowAlphaBar = true;
    FVector2 MinDesiredSize {220.0f, 176.0f};
};

class ImColorPicker : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImColorPicker, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::Color,
            "Color",
            static_cast<void (ImColorPicker::*)(FColor&)>(&ImColorPicker::SetColorProperty),
            static_cast<FColor& (ImColorPicker::*)()>(&ImColorPicker::GetColorProperty),
            "Current picked color")
        .RegisterProperty(PropertyType::Struct, "Style", &ImColorPicker::m_Style, "Color picker style");
    END_DECLARE_OBJECT()

public:
    using FColorChangedEvent = TMulticastDelegate<ImColorPicker&, const FColor&>;

    ImColorPicker();
    virtual ~ImColorPicker() = default;

    void SetColor(const FColor& color);
    const FColor& GetColor() const { return m_Color; }

    void SetStyle(const FColorPickerStyle& style);
    const FColorPickerStyle& GetStyle() const { return m_Style; }

    FColorChangedEvent OnColorChanged;
    FColorChangedEvent OnColorCommitted;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    enum class EActiveRegion {
        None,
        SaturationValue,
        Hue,
        Alpha
    };

    struct FPickerLayout {
        FGeometry SaturationValueRect;
        FGeometry HueRect;
        FGeometry AlphaRect;
        bool bHasAlphaRect = false;
    };

    void SetColorProperty(FColor& color) { SetColor(color); }
    FColor& GetColorProperty() { return m_Color; }

    FPickerLayout ResolveLayout() const;
    void SyncHsvFromColor();
    void SyncColorFromHsv(bool bBroadcastChanged);
    void UpdateFromMouse(const FVector2& mousePosition, bool bBroadcastChanged);
    bool BeginInteraction(const FVector2& mousePosition);
    void EndInteraction(bool bCommit);
    bool HandleKeyboardAdjust(const FInputEvent& event);
    float Clamp01(float value) const;

    FColorPickerStyle m_Style;
    FColor m_Color;
    float m_Hue = 0.0f;
    float m_Saturation = 1.0f;
    float m_Value = 1.0f;
    float m_Alpha = 1.0f;
    EActiveRegion m_ActiveRegion = EActiveRegion::None;
    bool m_bInteractionChanged = false;
};

} // namespace ImWidgetV4
