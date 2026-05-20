#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <string>

namespace ImWidgetV4 {

class ImButton : public ImPanelWidget {
public:
    using FButtonEvent = TMulticastDelegate<ImButton&>;

    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImButton"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImButton();
    virtual ~ImButton() = default;

    void SetContent(const Ptr& child);
    Ptr GetContent();
    ImPaddingSlot* GetContentSlot();

    void SetText(const std::string& text);
    void SetText(const FText& text);
    std::string GetText() const;

    void SetStyle(const FButtonStyle& style);
    const FButtonStyle& GetStyle() const { return GetEffectiveStyle(); }
    void SetNormalStyle(const FButtonStateStyle& style) { m_Style.Normal = style; }
    void SetHoveredStyle(const FButtonStateStyle& style) { m_Style.Hovered = style; }
    void SetPressedStyle(const FButtonStateStyle& style) { m_Style.Pressed = style; }
    void SetDisabledStyle(const FButtonStateStyle& style) { m_Style.Disabled = style; }

    void SetDisabled(bool bDisabled)
    {
        if (m_bDisabled == bDisabled) {
            return;
        }

        m_bDisabled = bDisabled;
        Invalidate(EInvalidateReason::Paint);
    }

    bool IsDisabled() const { return m_bDisabled; }
    bool IsHovered() const { return m_bHovered; }
    bool IsPressed() const { return m_bPressed; }

    FButtonEvent OnClicked;
    FButtonEvent OnPressed;
    FButtonEvent OnReleased;
    FButtonEvent OnHoverBegin;
    FButtonEvent OnHoverEnd;

    std::unique_ptr<ImSlot> CreateSlot() override;
    void Paint(const FPaintContext& paintContext) override;
    FVector2 GetMinSize() const override;
    FReply OnInputEvent(const FInputEvent& event) override;
    void OnFocusChanged(bool bHasFocus) override;
    virtual void Relayout();

protected:
    const FButtonStyle& GetEffectiveStyle() const;
    const FButtonStateStyle& GetCurrentStateStyle() const;
    void SetPressed(bool bPressed);
    void SetHovered(bool bHovered);
    void TriggerClick();
    void RenderButton(const FPaintContext& paintContext);

private:
    FButtonStyle m_Style;
    mutable FButtonStyle m_ResolvedThemeStyle;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;
    bool m_bHasExplicitStyle = false;
    FVector2 m_OriginalMinSize {100.0f, 30.0f};
};

} // namespace ImWidgetV4
