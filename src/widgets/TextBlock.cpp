#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <cfloat>

namespace ImWidgetV4 {

ImTextBlock::ImTextBlock()
    : ImWidget()
    , m_Text("")
    , m_TextColor(FColor::White)
    , m_FontSize(16.0f)
    , m_TextAlignment(ETextAlignment::Left)
    , m_VerticalAlignment(EVerticalAlignment::Top)
    , m_bWrapText(false)
{
}

void ImTextBlock::SetText(const std::string& text) {
    if (m_Text == text) {
        return;
    }

    m_Text = text;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
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
    Invalidate(EInvalidateReason::Paint);
}

void ImTextBlock::SetVerticalAlignment(EVerticalAlignment alignment) {
    if (m_VerticalAlignment == alignment) {
        return;
    }

    m_VerticalAlignment = alignment;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextBlock::SetWrapText(bool bWrap) {
    if (m_bWrapText == bWrap) {
        return;
    }

    m_bWrapText = bWrap;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextBlock::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible || m_Text.empty()) {
        return;
    }

    FVector2 textSize = CalculateTextSize();
    FVector2 textPos = CalculateTextPosition(textSize);

    if (m_bWrapText && m_Geometry.Size.X > 0.0f) {
        paintContext.DrawContext_.GetImDrawList()->AddText(
            nullptr,
            m_FontSize,
            textPos.ToImVec2(),
            m_TextColor.ToImU32(),
            m_Text.c_str(),
            nullptr,
            m_Geometry.Size.X
        );
    } else {
        paintContext.DrawContext_.DrawText(
            textPos,
            m_TextColor,
            m_Text,
            m_FontSize
        );
    }
}

FVector2 ImTextBlock::GetMinSize() const {
    return CalculateTextSize();
}

FVector2 ImTextBlock::CalculateTextSize() const {
    if (m_Text.empty()) {
        return FVector2(0.0f, m_FontSize);
    }

    if (ImGui::GetCurrentContext() == nullptr || ImGui::GetFont() == nullptr) {
        return FVector2(
            m_FontSize * 0.55f * static_cast<float>(m_Text.size()),
            m_FontSize);
    }

    const ImFont* font = ImGui::GetFont();
    ImVec2 size;
    if (m_bWrapText && m_Geometry.Size.X > 0.0f) {
        size = font->CalcTextSizeA(
            m_FontSize,
            FLT_MAX,
            m_Geometry.Size.X,
            m_Text.c_str(),
            nullptr,
            nullptr);
    } else {
        size = font->CalcTextSizeA(
            m_FontSize,
            FLT_MAX,
            0.0f,
            m_Text.c_str(),
            nullptr,
            nullptr);
    }

    return FVector2(size.x, size.y);
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

} // namespace ImWidgetV4
