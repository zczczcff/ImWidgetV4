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

struct FTextBlockStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FTextBlockStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor TextColor = FColor::White;
    float FontSize = 16.0f;
};

class ImTextBlock : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImTextBlock"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImTextBlock();
    virtual ~ImTextBlock() = default;

    void SetText(const std::string& text);
    void SetText(const FText& text);
    std::string GetText() const;
    const FText& GetTextValue() const { return m_TextValue; }

    void SetTextColor(const FColor& color);
    const FColor& GetTextColor() const { return GetEffectiveStyle().TextColor; }

    void SetFontSize(float size);
    float GetFontSize() const { return GetEffectiveStyle().FontSize; }

    void SetStyle(const FTextBlockStyle& style);
    const FTextBlockStyle& GetStyle() const { return GetEffectiveStyle(); }

    void SetTextAlignment(ETextAlignment alignment);
    ETextAlignment GetTextAlignment() const { return m_TextAlignment; }

    void SetVerticalAlignment(EVerticalAlignment alignment);
    EVerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

    void SetWrapText(bool bWrap);
    bool GetWrapText() const { return m_bWrapText; }

    void Paint(const FPaintContext& paintContext) override;
    FVector2 GetMinSize() const override;
    FReply OnInputEvent(const FInputEvent& event) override;
    void PostDeserializeFromJson() override;

private:
    const FTextBlockStyle& GetEffectiveStyle() const;
    void SetTextColorProperty(FColor& value) { SetTextColor(value); }
    FColor& GetTextColorProperty();
    void SetFontSizeProperty(float& value) { SetFontSize(value); }
    float& GetFontSizeProperty();
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
    FTextBlockStyle m_Style;
    mutable FTextBlockStyle m_ResolvedThemeStyle;
    FColor m_TextColorPropertyCache = FColor::White;
    float m_FontSizePropertyCache = 16.0f;
    ETextAlignment m_TextAlignment;
    EVerticalAlignment m_VerticalAlignment;
    bool m_bWrapText;
    bool m_bHasExplicitStyle = false;
    bool m_bHasExplicitTextColor = false;
    bool m_bHasExplicitFontSize = false;
    int m_TextAlignmentValue = 0;
    int m_VerticalAlignmentValue = 0;
};

} // namespace ImWidgetV4
