#pragma once

#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/core/Widget.h>
#include <string>

namespace ImWidgetV4 {

enum class ETextAlignment {
    Left,
    Center,
    Right
};

enum class EVerticalAlignment {
    Top,
    Center,
    Bottom
};

class ImTextBlock : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImTextBlock, ImWidget)
    registrar
        .RegisterProperty(PropertyType::String, "Text", &ImTextBlock::m_Text, "Displayed text")
        .RegisterProperty(PropertyType::Color, "TextColor", &ImTextBlock::m_TextColor, "Displayed text color")
        .RegisterProperty(PropertyType::Float, "FontSize", &ImTextBlock::m_FontSize, "Font size in pixels")
        .RegisterProperty(PropertyType::Bool, "WrapText", &ImTextBlock::m_bWrapText, "Whether the text wraps")
        .RegisterOptionalProperty(
            PropertyType::Enum,
            "TextAlignment",
            static_cast<void (ImTextBlock::*)(int&)>(&ImTextBlock::SetTextAlignmentProperty),
            static_cast<int& (ImTextBlock::*)()>(&ImTextBlock::GetTextAlignmentProperty),
            {"Left", "Center", "Right"},
            "Horizontal text alignment")
        .RegisterOptionalProperty(
            PropertyType::Enum,
            "VerticalAlignment",
            static_cast<void (ImTextBlock::*)(int&)>(&ImTextBlock::SetVerticalAlignmentProperty),
            static_cast<int& (ImTextBlock::*)()>(&ImTextBlock::GetVerticalAlignmentProperty),
            {"Top", "Center", "Bottom"},
            "Vertical text alignment");
    END_DECLARE_OBJECT()

public:
    ImTextBlock();
    virtual ~ImTextBlock() = default;

    void SetText(const std::string& text);
    void SetText(const FText& text);
    std::string GetText() const;
    const FText& GetTextValue() const { return m_TextValue; }

    void SetTextColor(const FColor& color);
    const FColor& GetTextColor() const { return m_TextColor; }

    void SetFontSize(float size);
    float GetFontSize() const { return m_FontSize; }

    void SetTextAlignment(ETextAlignment alignment);
    ETextAlignment GetTextAlignment() const { return m_TextAlignment; }

    void SetVerticalAlignment(EVerticalAlignment alignment);
    EVerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

    void SetWrapText(bool bWrap);
    bool GetWrapText() const { return m_bWrapText; }

    void Paint(const FPaintContext& paintContext) override;
    FVector2 GetMinSize() const override;
    FReply OnInputEvent(const FInputEvent& event) override;
    void FromJson(const json& j) override;

private:
    void SetTextAlignmentProperty(int& value);
    int& GetTextAlignmentProperty();
    void SetVerticalAlignmentProperty(int& value);
    int& GetVerticalAlignmentProperty();

    FVector2 CalculateTextSize() const;
    std::string ResolveText() const;
    FVector2 CalculateTextPosition(const FVector2& textSize) const;
    bool IsTextClipped() const;
    void UpdateOverflowToolTip();

    std::string m_Text;
    FText m_TextValue;
    FColor m_TextColor;
    float m_FontSize;
    ETextAlignment m_TextAlignment;
    EVerticalAlignment m_VerticalAlignment;
    bool m_bWrapText;
    int m_TextAlignmentValue = 0;
    int m_VerticalAlignmentValue = 0;
};

} // namespace ImWidgetV4
