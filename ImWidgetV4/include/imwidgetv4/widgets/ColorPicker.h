#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4 {

struct FColorPickerStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FColorPickerStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImColorPicker"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FColorChangedEvent = TMulticastDelegate<ImColorPicker&, const FColor&>;

    ImColorPicker();
    virtual ~ImColorPicker() = default;

    void SetColor(const FColor& color);
    const FColor& GetColor() const { return m_Color; }

    void SetStyle(const FColorPickerStyle& style);
    const FColorPickerStyle& GetStyle() const { return GetEffectiveStyle(); }

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
    const FColorPickerStyle& GetEffectiveStyle() const;
    float Clamp01(float value) const;

    FColorPickerStyle m_Style;
    mutable FColorPickerStyle m_ResolvedThemeStyle;
    FColor m_Color;
    float m_Hue = 0.0f;
    float m_Saturation = 1.0f;
    float m_Value = 1.0f;
    float m_Alpha = 1.0f;
    EActiveRegion m_ActiveRegion = EActiveRegion::None;
    bool m_bInteractionChanged = false;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
