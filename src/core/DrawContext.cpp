#include <imwidgetv4/core/DrawContext.h>

namespace ImWidgetV4 {

DrawContext::DrawContext(ImDrawList* drawList)
    : m_DrawList(drawList)
{
}

// ==================== 文本绘制 ====================

void DrawContext::DrawText(const FVector2& position, const FColor& color,
                           const std::string& text, float fontSize)
{
    if (!m_DrawList) return;

    ImFont* font = ImGui::GetFont();
    if (fontSize <= 0.0f) {
        fontSize = ImGui::GetFontSize();
    }

    m_DrawList->AddText(
        font,
        fontSize,
        position.ToImVec2(),
        color.ToImU32(),
        text.c_str()
    );
}

// ==================== 矩形绘制 ====================

void DrawContext::DrawRect(const FVector2& min, const FVector2& max,
                           const FColor& color, float rounding,
                           float thickness)
{
    if (!m_DrawList) return;

    m_DrawList->AddRect(
        min.ToImVec2(),
        max.ToImVec2(),
        color.ToImU32(),
        rounding,
        ImDrawFlags_None,
        thickness
    );
}

void DrawContext::DrawRectFilled(const FVector2& min, const FVector2& max,
                                 const FColor& color, float rounding)
{
    if (!m_DrawList) return;

    m_DrawList->AddRectFilled(
        min.ToImVec2(),
        max.ToImVec2(),
        color.ToImU32(),
        rounding
    );
}

// ==================== 线条绘制 ====================

void DrawContext::DrawLine(const FVector2& p1, const FVector2& p2,
                           const FColor& color, float thickness)
{
    if (!m_DrawList) return;

    m_DrawList->AddLine(
        p1.ToImVec2(),
        p2.ToImVec2(),
        color.ToImU32(),
        thickness
    );
}

// ==================== 圆形绘制 ====================

void DrawContext::DrawCircle(const FVector2& center, float radius,
                             const FColor& color, int numSegments,
                             float thickness)
{
    if (!m_DrawList) return;

    m_DrawList->AddCircle(
        center.ToImVec2(),
        radius,
        color.ToImU32(),
        numSegments,
        thickness
    );
}

void DrawContext::DrawCircleFilled(const FVector2& center, float radius,
                                   const FColor& color, int numSegments)
{
    if (!m_DrawList) return;

    m_DrawList->AddCircleFilled(
        center.ToImVec2(),
        radius,
        color.ToImU32(),
        numSegments
    );
}

// ==================== 图片绘制 ====================

void DrawContext::DrawImage(ImTextureID textureId, const FVector2& min,
                            const FVector2& max, const FVector2& uvMin,
                            const FVector2& uvMax, const FColor& tintColor)
{
    if (!m_DrawList) return;

    m_DrawList->AddImage(
        textureId,
        min.ToImVec2(),
        max.ToImVec2(),
        uvMin.ToImVec2(),
        uvMax.ToImVec2(),
        tintColor.ToImU32()
    );
}

// ==================== 路径绘制 ====================

void DrawContext::PathLineTo(const FVector2& pos)
{
    if (!m_DrawList) return;

    m_DrawList->PathLineTo(pos.ToImVec2());
}

void DrawContext::PathArcTo(const FVector2& center, float radius,
                            float aMin, float aMax, int numSegments)
{
    if (!m_DrawList) return;

    m_DrawList->PathArcTo(
        center.ToImVec2(),
        radius,
        aMin,
        aMax,
        numSegments
    );
}

void DrawContext::PathStroke(const FColor& color, bool closed,
                             float thickness)
{
    if (!m_DrawList) return;

    m_DrawList->PathStroke(
        color.ToImU32(),
        closed ? ImDrawFlags_Closed : ImDrawFlags_None,
        thickness
    );
}

void DrawContext::PathFill(const FColor& color)
{
    if (!m_DrawList) return;

    m_DrawList->PathFillConvex(color.ToImU32());
}

// ==================== 裁剪区域 ====================

void DrawContext::PushClipRect(const FVector2& min, const FVector2& max,
                               bool intersectWithCurrent)
{
    if (!m_DrawList) return;

    m_DrawList->PushClipRect(
        min.ToImVec2(),
        max.ToImVec2(),
        intersectWithCurrent
    );
}

void DrawContext::PopClipRect()
{
    if (!m_DrawList) return;

    m_DrawList->PopClipRect();
}

} // namespace ImWidgetV4
