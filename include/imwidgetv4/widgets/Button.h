#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <string>

namespace ImWidgetV4 {

class ImButton : public ImPanelWidget {
public:
    using FButtonEvent = TMulticastDelegate<ImButton&>;

    ImButton();
    virtual ~ImButton() = default;

    void SetContent(const Ptr& child);
    Ptr GetContent();
    ImPaddingSlot* GetContentSlot();

    void SetText(const std::string& text);
    std::string GetText() const;

    void SetStyle(const FButtonStyle& style) { m_Style = style; }
    const FButtonStyle& GetStyle() const { return m_Style; }
    void SetNormalStyle(const FButtonStateStyle& style) { m_Style.Normal = style; }
    void SetHoveredStyle(const FButtonStateStyle& style) { m_Style.Hovered = style; }
    void SetPressedStyle(const FButtonStateStyle& style) { m_Style.Pressed = style; }
    void SetDisabledStyle(const FButtonStateStyle& style) { m_Style.Disabled = style; }

    void SetDisabled(bool bDisabled) {
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

    virtual std::unique_ptr<ImSlot> CreateSlot() override;
    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual void Relayout();

protected:
    const FButtonStateStyle& GetCurrentStateStyle() const;
    void SetPressed(bool bPressed);
    void SetHovered(bool bHovered);
    void TriggerClick();
    void RenderButton(const FPaintContext& paintContext);

private:
    FButtonStyle m_Style;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;
    FVector2 m_OriginalMinSize {100.0f, 30.0f};
};

} // namespace ImWidgetV4
