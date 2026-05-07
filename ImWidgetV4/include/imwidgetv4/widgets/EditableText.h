#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <cstddef>
#include <string>

namespace ImWidgetV4 {

struct FEditableTextStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FEditableTextStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FEditableTextStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "HoveredBackgroundColor", &FEditableTextStyle::HoveredBackgroundColor, "Hovered background color")
        .RegisterProperty(PropertyType::Color, "FocusedBackgroundColor", &FEditableTextStyle::FocusedBackgroundColor, "Focused background color")
        .RegisterProperty(PropertyType::Color, "DisabledBackgroundColor", &FEditableTextStyle::DisabledBackgroundColor, "Disabled background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FEditableTextStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FEditableTextStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "TextColor", &FEditableTextStyle::TextColor, "Text color")
        .RegisterProperty(PropertyType::Color, "DisabledTextColor", &FEditableTextStyle::DisabledTextColor, "Disabled text color")
        .RegisterProperty(PropertyType::Color, "HintTextColor", &FEditableTextStyle::HintTextColor, "Hint text color")
        .RegisterProperty(PropertyType::Color, "CaretColor", &FEditableTextStyle::CaretColor, "Caret color")
        .RegisterProperty(PropertyType::Color, "SelectionBackgroundColor", &FEditableTextStyle::SelectionBackgroundColor, "Selection background color")
        .RegisterProperty(PropertyType::Color, "SelectedTextColor", &FEditableTextStyle::SelectedTextColor, "Selected text color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FEditableTextStyle::Padding, "Text padding")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FEditableTextStyle::MinDesiredSize, "Minimum desired size")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FEditableTextStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FEditableTextStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "FontSize", &FEditableTextStyle::FontSize, "Font size");
    END_DECLARE_OBJECT()

public:
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
    FColor SelectionBackgroundColor = FColor::FromBytes(93, 149, 212, 176);
    FColor SelectedTextColor = FColor::FromBytes(248, 250, 252);
    FMargin Padding = FMargin(12.0f);
    FVector2 MinDesiredSize {228.0f, 40.0f};
    float CornerRadius = 7.0f;
    float BorderThickness = 1.0f;
    float FontSize = 16.0f;
};

class ImEditableText : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImEditableText, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::String,
            "Text",
            static_cast<void (ImEditableText::*)(std::string&)>(&ImEditableText::SetTextProperty),
            static_cast<std::string& (ImEditableText::*)()>(&ImEditableText::GetTextProperty),
            "Editable text content")
        .RegisterProperty(
            PropertyType::String,
            "HintText",
            static_cast<void (ImEditableText::*)(std::string&)>(&ImEditableText::SetHintTextProperty),
            static_cast<std::string& (ImEditableText::*)()>(&ImEditableText::GetHintTextProperty),
            "Hint text shown when empty")
        .RegisterProperty(PropertyType::Bool, "Disabled", &ImEditableText::m_bDisabled, "Whether the input is disabled")
        .RegisterProperty(PropertyType::Struct, "Style", &ImEditableText::m_Style, "Editable text style");
    END_DECLARE_OBJECT()

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
    bool HasSelection() const { return m_SelectionAnchorByteIndex != m_CursorByteIndex; }
    std::size_t GetSelectionStartByteIndex() const;
    std::size_t GetSelectionEndByteIndex() const;
    float GetHorizontalScrollOffset() const { return m_HorizontalScrollOffset; }
    void SelectAll();

    FTextEvent OnTextChanged;
    FTextEvent OnTextCommitted;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    void SetTextProperty(std::string& text) { SetText(text); }
    std::string& GetTextProperty() { return m_Text; }
    void SetHintTextProperty(std::string& hintText) { SetHintText(hintText); }
    std::string& GetHintTextProperty() { return m_HintText; }

    std::size_t ClampByteIndex(std::size_t byteIndex) const;
    std::size_t ResolveCursorByteIndexAt(const FVector2& mousePosition) const;
    void SetCursorByteIndex(std::size_t byteIndex, bool bExtendSelection = false);
    void EnsureCursorVisible();
    void NotifyTextChanged();
    void CommitText();
    void InsertCodepoint(unsigned int codepoint);
    void InsertText(const std::string& text);
    void DeleteSelection();
    void DeletePreviousCodepoint();
    void DeleteNextCodepoint();
    void DeletePreviousWord();
    void DeleteNextWord();
    void MoveCursorLeft(bool bExtendSelection = false);
    void MoveCursorRight(bool bExtendSelection = false);
    void MoveCursorWordLeft(bool bExtendSelection = false);
    void MoveCursorWordRight(bool bExtendSelection = false);
    void MoveCursorToStart(bool bExtendSelection = false);
    void MoveCursorToEnd(bool bExtendSelection = false);
    std::string GetSelectedText() const;
    void CopySelectionToClipboard() const;
    void CutSelectionToClipboard();
    void PasteFromClipboard();
    float MeasureCaretX(std::size_t byteIndex) const;
    FVector2 MeasureText(const std::string& text) const;
    void SetHovered(bool bHovered);
    void SetDraggingSelection(bool bDraggingSelection);

    FEditableTextStyle m_Style;
    std::string m_Text;
    std::string m_HintText;
    std::size_t m_CursorByteIndex = 0;
    std::size_t m_SelectionAnchorByteIndex = 0;
    float m_HorizontalScrollOffset = 0.0f;
    bool m_bHovered = false;
    bool m_bDisabled = false;
    bool m_bDraggingSelection = false;
    bool m_bTextDirty = false;
};

} // namespace ImWidgetV4
