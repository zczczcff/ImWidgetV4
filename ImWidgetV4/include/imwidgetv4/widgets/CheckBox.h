#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Widget.h>
#include <string>

namespace ImWidgetV4 {

struct FCheckBoxStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FCheckBoxStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImCheckBox"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FCheckBoxEvent = TMulticastDelegate<ImCheckBox&>;
    using FCheckStateChangedEvent = TMulticastDelegate<ImCheckBox&, bool>;

    ImCheckBox();
    virtual ~ImCheckBox() = default;

    void SetLabel(const std::string& label);
    void SetLabel(const FText& label);
    std::string GetLabel() const;
    const FText& GetLabelText() const { return m_LabelText; }

    void SetChecked(bool bChecked);
    bool IsChecked() const { return m_bChecked; }
    void Toggle();

    void SetStyle(const FCheckBoxStyle& style);
    const FCheckBoxStyle& GetStyle() const { return GetEffectiveStyle(); }

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
    void FromJson(const json& j) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    const FCheckBoxStyle& GetEffectiveStyle() const;
    void SetPressed(bool bPressed);
    void SetHovered(bool bHovered);
    FVector2 MeasureLabelSize() const;
    std::string ResolveLabel() const;

    FCheckBoxStyle m_Style;
    mutable FCheckBoxStyle m_ResolvedThemeStyle;
    std::string m_Label;
    FText m_LabelText;
    bool m_bChecked = false;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
