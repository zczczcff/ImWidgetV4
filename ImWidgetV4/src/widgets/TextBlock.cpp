#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <algorithm>
#include <imgui.h>
#include <cfloat>

namespace ImWidgetV4 {

ImTextBlock::ImTextBlock()
    : ImWidget()
    , m_Text("")
    , m_TextValue(FText::FromString(""))
    , m_TextAlignment(ETextAlignment::Center)
    , m_VerticalAlignment(EVerticalAlignment::Center)
    , m_bWrapText(false)
    , m_TextAlignmentValue(static_cast<int>(ETextAlignment::Center))
    , m_VerticalAlignmentValue(static_cast<int>(EVerticalAlignment::Center))
{
}

void ImTextBlock::SetText(const std::string& text) {
    SetText(FText::FromString(text));
}

void ImTextBlock::SetText(const FText& text) {
    if (m_TextValue == text) {
        return;
    }

    m_TextValue = text;
    if (text.IsLocalized()) {
        m_Text = text.GetDefaultText().empty() ? text.GetKey() : text.GetDefaultText();
    } else {
        m_Text = text.GetInvariantText();
    }
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

std::string ImTextBlock::GetText() const
{
    return ResolveText();
}

void ImTextBlock::SetTextColor(const FColor& color) {
    const FTextBlockStyle effectiveStyle = GetEffectiveStyle();
    if (effectiveStyle.TextColor.ToImU32() == color.ToImU32()) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = effectiveStyle;
    }
    m_Style.TextColor = color;
    m_bHasExplicitTextColor = true;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextBlock::SetFontSize(float size) {
    const float clampedSize = std::max(1.0f, size);
    if (GetEffectiveStyle().FontSize == clampedSize) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = GetEffectiveStyle();
    }
    m_Style.FontSize = clampedSize;
    m_bHasExplicitFontSize = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextBlock::SetStyle(const FTextBlockStyle& style)
{
    m_Style = style;
    m_bHasExplicitStyle = true;
    m_bHasExplicitTextColor = true;
    m_bHasExplicitFontSize = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextBlock::SetTextAlignment(ETextAlignment alignment) {
    if (m_TextAlignment == alignment) {
        return;
    }

    m_TextAlignment = alignment;
    m_TextAlignmentValue = static_cast<int>(alignment);
    Invalidate(EInvalidateReason::Paint);
}

void ImTextBlock::SetVerticalAlignment(EVerticalAlignment alignment) {
    if (m_VerticalAlignment == alignment) {
        return;
    }

    m_VerticalAlignment = alignment;
    m_VerticalAlignmentValue = static_cast<int>(alignment);
    Invalidate(EInvalidateReason::Paint);
}

void ImTextBlock::SetWrapText(bool bWrap) {
    if (m_bWrapText == bWrap) {
        return;
    }

    m_bWrapText = bWrap;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextBlock::SetTextAlignmentProperty(int& value) {
    value = std::clamp(value, 0, 2);
    SetTextAlignment(static_cast<ETextAlignment>(value));
}

FColor& ImTextBlock::GetTextColorProperty()
{
    m_TextColorPropertyCache = GetTextColor();
    return m_TextColorPropertyCache;
}

float& ImTextBlock::GetFontSizeProperty()
{
    m_FontSizePropertyCache = GetFontSize();
    return m_FontSizePropertyCache;
}

int& ImTextBlock::GetTextAlignmentProperty() {
    m_TextAlignmentValue = static_cast<int>(m_TextAlignment);
    return m_TextAlignmentValue;
}

void ImTextBlock::SetVerticalAlignmentProperty(int& value) {
    value = std::clamp(value, 0, 2);
    SetVerticalAlignment(static_cast<EVerticalAlignment>(value));
}

int& ImTextBlock::GetVerticalAlignmentProperty() {
    m_VerticalAlignmentValue = static_cast<int>(m_VerticalAlignment);
    return m_VerticalAlignmentValue;
}

void ImTextBlock::Paint(const FPaintContext& paintContext) {
    const std::string text = ResolveText();
    if (!m_bVisible || text.empty()) {
        ClearToolTip();
        return;
    }

    UpdateOverflowToolTip();

    const FTextBlockStyle& style = GetEffectiveStyle();
    FVector2 textSize = CalculateTextSize();
    FVector2 textPos = CalculateTextPosition(textSize);

    paintContext.DrawContext_.PushClipRect(m_Geometry.GetMin(), m_Geometry.GetMax(), true);
    paintContext.DrawContext_.DrawText(
        textPos,
        style.TextColor,
        text,
        style.FontSize
    );
    paintContext.DrawContext_.PopClipRect();
}

FVector2 ImTextBlock::GetMinSize() const {
    return CalculateTextSize();
}

FReply ImTextBlock::OnInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::MouseEnter ||
        event.Type == EInputEventType::MouseMove) {
        UpdateOverflowToolTip();
    }

    return FReply::Unhandled();
}

FVector2 ImTextBlock::CalculateTextSize() const {
    const std::string text = ResolveText();
    if (text.empty()) {
        return FVector2(0.0f, GetEffectiveStyle().FontSize);
    }

    const float fontSize = GetEffectiveStyle().FontSize;

    if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr) {
        return FVector2(
            fontSize * 0.55f * static_cast<float>(text.size()),
            fontSize);
    }

    const ImFont* font = ImGui::GetFont();
    const ImVec2 size = font->CalcTextSizeA(
        fontSize,
        FLT_MAX,
        0.0f,
        text.c_str(),
        nullptr,
        nullptr);

    return FVector2(size.x, size.y);
}

const FTextBlockStyle& ImTextBlock::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveTextBlockStyle(application->GetStyleSet());
        if (m_bHasExplicitTextColor) {
            m_ResolvedThemeStyle.TextColor = m_Style.TextColor;
        }
        if (m_bHasExplicitFontSize) {
            m_ResolvedThemeStyle.FontSize = m_Style.FontSize;
        }
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

std::string ImTextBlock::ResolveText() const
{
    return m_TextValue.Resolve();
}

FVector2 ImTextBlock::CalculateTextPosition(const FVector2& textSize) const {
    FVector2 pos = m_Geometry.Position;

    switch (m_TextAlignment) {
        case ETextAlignment::Center:
            pos.X += (m_Geometry.Size.X - textSize.X) * 0.5f;
            break;
        case ETextAlignment::Right:
            pos.X += m_Geometry.Size.X - textSize.X;
            break;
        case ETextAlignment::Left:
        default:
            break;
    }

    switch (m_VerticalAlignment) {
        case EVerticalAlignment::Center:
            pos.Y += (m_Geometry.Size.Y - textSize.Y) * 0.5f;
            break;
        case EVerticalAlignment::Bottom:
            pos.Y += m_Geometry.Size.Y - textSize.Y;
            break;
        case EVerticalAlignment::Top:
        default:
            break;
    }

    return pos;
}

bool ImTextBlock::IsTextClipped() const
{
    if (ResolveText().empty() || m_Geometry.Size.X <= 0.0f || m_Geometry.Size.Y <= 0.0f) {
        return false;
    }

    const FVector2 textSize = CalculateTextSize();
    return textSize.X > m_Geometry.Size.X + 0.5f ||
           textSize.Y > m_Geometry.Size.Y + 0.5f;
}

void ImTextBlock::UpdateOverflowToolTip()
{
    const std::string text = ResolveText();
    if (IsTextClipped()) {
        SetToolTipText(m_TextValue);
    } else if (GetToolTipText() == text && GetToolTipWidget() == nullptr) {
        ClearToolTip();
    }
}

void ImTextBlock::FromJson(const json& j)
{
    ImWidget::FromJson(j);
    m_TextValue = FText::FromString(m_Text);
}

} // namespace ImWidgetV4
