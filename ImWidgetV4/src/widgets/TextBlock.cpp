#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>
#include <imgui.h>
#include <cfloat>

namespace ImWidgetV4 {

ImTextBlock::ImTextBlock()
    : ImWidget()
    , m_Text("")
    , m_TextValue(FText::FromString(""))
    , m_TextColor(FColor::White)
    , m_FontSize(16.0f)
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
    if (m_TextColor.R == color.R &&
        m_TextColor.G == color.G &&
        m_TextColor.B == color.B &&
        m_TextColor.A == color.A) {
        return;
    }

    m_TextColor = color;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextBlock::SetFontSize(float size) {
    if (m_FontSize == size) {
        return;
    }

    m_FontSize = size;
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

    FVector2 textSize = CalculateTextSize();
    FVector2 textPos = CalculateTextPosition(textSize);

    paintContext.DrawContext_.PushClipRect(m_Geometry.GetMin(), m_Geometry.GetMax(), true);
    paintContext.DrawContext_.DrawText(
        textPos,
        m_TextColor,
        text,
        m_FontSize
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
        return FVector2(0.0f, m_FontSize);
    }

    if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr) {
        return FVector2(
            m_FontSize * 0.55f * static_cast<float>(text.size()),
            m_FontSize);
    }

    const ImFont* font = ImGui::GetFont();
    const ImVec2 size = font->CalcTextSizeA(
        m_FontSize,
        FLT_MAX,
        0.0f,
        text.c_str(),
        nullptr,
        nullptr);

    return FVector2(size.x, size.y);
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
