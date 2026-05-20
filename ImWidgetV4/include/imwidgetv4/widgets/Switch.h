#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>

namespace ImWidgetV4 {

struct FSwitchStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FSwitchStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor OffTrackColor = FColor::FromBytes(57, 64, 75);
    FColor OffTrackHoveredColor = FColor::FromBytes(73, 82, 95);
    FColor OffTrackPressedColor = FColor::FromBytes(43, 49, 58);
    FColor OnTrackColor = FColor::FromBytes(78, 126, 196);
    FColor OnTrackHoveredColor = FColor::FromBytes(96, 149, 221);
    FColor OnTrackPressedColor = FColor::FromBytes(63, 108, 177);
    FColor DisabledTrackColor = FColor::FromBytes(74, 79, 87);
    FColor ThumbColor = FColor::FromBytes(248, 250, 252);
    FColor ThumbHoveredColor = FColor::White;
    FColor ThumbPressedColor = FColor::FromBytes(255, 214, 102);
    FColor DisabledThumbColor = FColor::FromBytes(170, 176, 184);
    FColor BorderColor = FColor::FromBytes(19, 23, 29);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    float BorderThickness = 1.0f;
    float ThumbInset = 3.0f;
    FVector2 DesiredSize {52.0f, 28.0f};
};

class ImSwitch : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImSwitch"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FSwitchEvent = TMulticastDelegate<ImSwitch&>;
    using FCheckStateChangedEvent = TMulticastDelegate<ImSwitch&, bool>;

    ImSwitch();
    virtual ~ImSwitch() = default;

    void SetChecked(bool bChecked);
    bool IsChecked() const { return m_bChecked; }
    void Toggle();

    void SetEnabled(bool bEnabled);
    bool IsEnabled() const { return !m_bDisabled; }

    void SetDisabled(bool bDisabled);
    bool IsDisabled() const { return m_bDisabled; }

    void SetStyle(const FSwitchStyle& style);
    const FSwitchStyle& GetStyle() const { return GetEffectiveStyle(); }

    bool IsHovered() const { return m_bHovered; }
    bool IsPressed() const { return m_bPressed; }

    FCheckStateChangedEvent OnCheckStateChanged;
    FSwitchEvent OnPressed;
    FSwitchEvent OnReleased;
    FSwitchEvent OnHoverBegin;
    FSwitchEvent OnHoverEnd;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    const FSwitchStyle& GetEffectiveStyle() const;
    void SetHovered(bool bHovered);
    void SetPressed(bool bPressed);
    FColor ResolveTrackColor() const;
    FColor ResolveThumbColor() const;
    FGeometry GetTrackGeometry() const;
    FVector2 GetThumbCenter() const;
    float GetThumbRadius() const;

    FSwitchStyle m_Style;
    mutable FSwitchStyle m_ResolvedThemeStyle;
    bool m_bChecked = false;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
