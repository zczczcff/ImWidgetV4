#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>

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
    m_Text = text;
}

void ImTextBlock::SetTextColor(const FColor& color) {
    m_TextColor = color;
}

void ImTextBlock::SetFontSize(float size) {
    m_FontSize = size;
}

void ImTextBlock::SetTextAlignment(ETextAlignment alignment) {
    m_TextAlignment = alignment;
}

void ImTextBlock::SetVerticalAlignment(EVerticalAlignment alignment) {
    m_VerticalAlignment = alignment;
}

void ImTextBlock::SetWrapText(bool bWrap) {
    m_bWrapText = bWrap;
}

void ImTextBlock::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible || m_Text.empty()) {
        return;
    }

    // 计算文本尺寸
    FVector2 textSize = CalculateTextSize();

    // 计算文本位置（考虑对齐方式）
    FVector2 textPos = CalculateTextPosition(textSize);

    // 使用 DrawContext 绘制文本
    if (m_bWrapText && m_Geometry.Size.X > 0.0f) {
        // 带换行的文本需要直接访问 ImDrawList（DrawContext 暂不支持换行参数）
        paintContext.DrawContext_.GetImDrawList()->AddText(
            nullptr,                        // 使用默认字体
            m_FontSize,                     // 字体大小
            textPos.ToImVec2(),             // 文本位置
            m_TextColor.ToImU32(),          // 文本颜色
            m_Text.c_str(),                 // 文本内容
            nullptr,                        // 文本结束位置（nullptr 表示自动检测）
            m_Geometry.Size.X               // 换行宽度
        );
    } else {
        // 单行文本使用封装的 DrawContext 接口
        paintContext.DrawContext_.DrawText(
            textPos,                        // 文本位置
            m_TextColor,                    // 文本颜色
            m_Text,                         // 文本内容
            m_FontSize                      // 字体大小
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

    // 使用 ImGui 计算文本尺寸
    ImVec2 size;
    if (m_bWrapText && m_Geometry.Size.X > 0.0f) {
        // 带换行的文本尺寸计算
        size = ImGui::CalcTextSize(
            m_Text.c_str(),
            nullptr,
            false,
            m_Geometry.Size.X
        );
    } else {
        // 单行文本尺寸计算
        size = ImGui::CalcTextSize(m_Text.c_str());
    }

    return FVector2(size.x, size.y);
}

FVector2 ImTextBlock::CalculateTextPosition(const FVector2& textSize) const {
    FVector2 pos = m_Geometry.Position;

    // 水平对齐
    switch (m_TextAlignment) {
        case ETextAlignment::Center:
            pos.X += (m_Geometry.Size.X - textSize.X) * 0.5f;
            break;
        case ETextAlignment::Right:
            pos.X += m_Geometry.Size.X - textSize.X;
            break;
        case ETextAlignment::Left:
        default:
            // 保持左对齐，不需要调整
            break;
    }

    // 垂直对齐
    switch (m_VerticalAlignment) {
        case EVerticalAlignment::Center:
            pos.Y += (m_Geometry.Size.Y - textSize.Y) * 0.5f;
            break;
        case EVerticalAlignment::Bottom:
            pos.Y += m_Geometry.Size.Y - textSize.Y;
            break;
        case EVerticalAlignment::Top:
        default:
            // 保持顶部对齐，不需要调整
            break;
    }

    return pos;
}

} // namespace ImWidgetV4
