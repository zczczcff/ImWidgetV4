#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <cstddef>
#include <string>

namespace ImWidgetV4 {

struct FEditableTextStyle {
    FColor BackgroundColor = FColor::FromBytes(31, 37, 46);
    FColor HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
    FColor FocusedBackgroundColor = FColor::FromBytes(24, 31, 40);
    FColor DisabledBackgroundColor = FColor::FromBytes(56, 60, 66);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor TextColor = FColor::FromBytes(238, 241, 245);
    FColor DisabledTextColor = FColor::FromBytes(140, 147, 156);
    FColor HintTextColor = FColor::FromBytes(135, 145, 157);
    FColor CaretColor = FColor::FromBytes(245, 247, 250);
    FMargin Padding = FMargin(12.0f);
    FVector2 MinDesiredSize {228.0f, 40.0f};
    float CornerRadius = 7.0f;
    float BorderThickness = 1.0f;
    float FontSize = 16.0f;
};

class ImEditableText : public ImWidget {
public:
    using FTextEvent = TMulticastDelegate<ImEditableText&, const std::string&>;

    ImEditableText();
    virtual ~ImEditableText() = default;

    void SetText(const std::string& text);
    const std::string& GetText() const { return m_Text; }

    void SetHintText(const std::string& hintText);
    const std::string& GetHintText() const { return m_HintText; }

    void SetStyle(const FEditableTextStyle& style);
    const FEditableTextStyle& GetStyle() const { return m_Style; }

    void SetDisabled(bool bDisabled);
    bool IsDisabled() const { return m_bDisabled; }
    bool IsHovered() const { return m_bHovered; }
    std::size_t GetCursorByteIndex() const { return m_CursorByteIndex; }
    float GetHorizontalScrollOffset() const { return m_HorizontalScrollOffset; }

    FTextEvent OnTextChanged;
    FTextEvent OnTextCommitted;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    std::size_t ClampByteIndex(std::size_t byteIndex) const;
    std::size_t ResolveCursorByteIndexAt(const FVector2& mousePosition) const;
    void SetCursorByteIndex(std::size_t byteIndex);
    void EnsureCursorVisible();
    void NotifyTextChanged();
    void CommitText();
    void InsertCodepoint(unsigned int codepoint);
    void InsertText(const std::string& text);
    void DeletePreviousCodepoint();
    void DeleteNextCodepoint();
    void MoveCursorLeft();
    void MoveCursorRight();
    void MoveCursorToStart();
    void MoveCursorToEnd();
    float MeasureCaretX(std::size_t byteIndex) const;
    FVector2 MeasureText(const std::string& text) const;
    void SetHovered(bool bHovered);

    FEditableTextStyle m_Style;
    std::string m_Text;
    std::string m_HintText;
    std::size_t m_CursorByteIndex = 0;
    float m_HorizontalScrollOffset = 0.0f;
    bool m_bHovered = false;
    bool m_bDisabled = false;
    bool m_bTextDirty = false;
};

} // namespace ImWidgetV4
