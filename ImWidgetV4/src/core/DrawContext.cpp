#include <imwidgetv4/core/DrawContext.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

namespace {

bool IsFiniteFloat(float value)
{
    return std::isfinite(static_cast<double>(value));
}

bool IsFiniteVector(const FVector2& value)
{
    return IsFiniteFloat(value.X) && IsFiniteFloat(value.Y);
}

bool SanitizeRect(
    const FVector2& min,
    const FVector2& max,
    FVector2& outMin,
    FVector2& outMax,
    float& inOutRounding)
{
    if (!IsFiniteVector(min) || !IsFiniteVector(max) || !IsFiniteFloat(inOutRounding)) {
        return false;
    }

    outMin = FVector2((std::min)(min.X, max.X), (std::min)(min.Y, max.Y));
    outMax = FVector2((std::max)(min.X, max.X), (std::max)(min.Y, max.Y));

    const float width = outMax.X - outMin.X;
    const float height = outMax.Y - outMin.Y;
    if (width <= 0.0f || height <= 0.0f) {
        return false;
    }

    const float maxRounding = (std::min)(width, height) * 0.5f;
    inOutRounding = (std::clamp)(inOutRounding, 0.0f, maxRounding);
    return true;
}

float ResolveSafeRounding(float rounding)
{
    if (!IsFiniteFloat(rounding) || rounding <= 0.5f) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() == nullptr) {
        return 0.0f;
    }

    const ImDrawListSharedData* sharedData = ImGui::GetDrawListSharedData();
    if (sharedData == nullptr) {
        return 0.0f;
    }

    if (!IsFiniteFloat(sharedData->CircleSegmentMaxError) ||
        sharedData->CircleSegmentMaxError <= 0.0f) {
        return 0.0f;
    }

    const int radiusIndex = static_cast<int>(std::ceil(rounding));
    constexpr int kSegmentCountCount = static_cast<int>(
        sizeof(sharedData->CircleSegmentCounts) / sizeof(sharedData->CircleSegmentCounts[0]));
    if (radiusIndex >= 0 &&
        radiusIndex < kSegmentCountCount &&
        sharedData->CircleSegmentCounts[radiusIndex] == 0) {
        return 0.0f;
    }

    return rounding;
}

ImFont* ResolveFontForDraw()
{
    if (ImGui::GetCurrentContext() == nullptr) {
        return nullptr;
    }

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = ImGui::GetFont();
    if (font == nullptr) {
        font = io.FontDefault;
    }
    if (font == nullptr && io.Fonts != nullptr && !io.Fonts->Fonts.empty()) {
        font = io.Fonts->Fonts[0];
    }
    if (font == nullptr || font->ContainerAtlas == nullptr) {
        return nullptr;
    }

    return font;
}

} // namespace

DrawContext::DrawContext(ImDrawList* drawList)
    : m_DrawList(drawList)
{
}

void DrawContext::DrawText(const FVector2& position, const FColor& color,
                           const std::string& text, float fontSize)
{
    if (!m_DrawList || ImGui::GetCurrentContext() == nullptr || text.empty()) {
        return;
    }

    ImFont* font = ResolveFontForDraw();
    if (font == nullptr) {
        return;
    }

    if (fontSize <= 0.0f) {
        fontSize = ImGui::GetFontSize();
    }
    if (fontSize <= 0.0f) {
        fontSize = font->FontSize;
    }
    if (!IsFiniteFloat(fontSize) || fontSize <= 0.0f) {
        return;
    }

    const ImTextureID fontTextureId = font->ContainerAtlas->TexID;
    const bool bPushTexture = fontTextureId != nullptr && m_DrawList->_CmdHeader.TextureId != fontTextureId;
    if (bPushTexture) {
        m_DrawList->PushTextureID(fontTextureId);
    }

    const ImVec4 clipRect = m_DrawList->_CmdHeader.ClipRect;
    const char* textBegin = text.c_str();
    const char* textEnd = textBegin + text.size();
    font->RenderText(
        m_DrawList,
        fontSize,
        position.ToImVec2(),
        color.ToImU32(),
        clipRect,
        textBegin,
        textEnd,
        0.0f,
        false);

    if (bPushTexture) {
        m_DrawList->PopTextureID();
    }
}

void DrawContext::DrawRect(const FVector2& min, const FVector2& max,
                           const FColor& color, float rounding,
                           float thickness)
{
    if (!m_DrawList) return;

    FVector2 sanitizedMin;
    FVector2 sanitizedMax;
    if (!SanitizeRect(min, max, sanitizedMin, sanitizedMax, rounding)) {
        return;
    }
    rounding = ResolveSafeRounding(rounding);

    if (!IsFiniteFloat(thickness) || thickness <= 0.0f) {
        thickness = 1.0f;
    }

    m_DrawList->AddRect(
        sanitizedMin.ToImVec2(),
        sanitizedMax.ToImVec2(),
        color.ToImU32(),
        rounding,
        ImDrawFlags_None,
        thickness);
}

void DrawContext::DrawRectFilled(const FVector2& min, const FVector2& max,
                                 const FColor& color, float rounding)
{
    if (!m_DrawList) return;

    FVector2 sanitizedMin;
    FVector2 sanitizedMax;
    if (!SanitizeRect(min, max, sanitizedMin, sanitizedMax, rounding)) {
        return;
    }
    rounding = ResolveSafeRounding(rounding);

    m_DrawList->AddRectFilled(
        sanitizedMin.ToImVec2(),
        sanitizedMax.ToImVec2(),
        color.ToImU32(),
        rounding);
}

void DrawContext::DrawLine(const FVector2& p1, const FVector2& p2,
                           const FColor& color, float thickness)
{
    if (!m_DrawList) return;

    m_DrawList->AddLine(
        p1.ToImVec2(),
        p2.ToImVec2(),
        color.ToImU32(),
        thickness);
}

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
        thickness);
}

void DrawContext::DrawCircleFilled(const FVector2& center, float radius,
                                   const FColor& color, int numSegments)
{
    if (!m_DrawList) return;

    m_DrawList->AddCircleFilled(
        center.ToImVec2(),
        radius,
        color.ToImU32(),
        numSegments);
}

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
        tintColor.ToImU32());
}

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
        numSegments);
}

void DrawContext::PathStroke(const FColor& color, bool closed,
                             float thickness)
{
    if (!m_DrawList) return;

    m_DrawList->PathStroke(
        color.ToImU32(),
        closed ? ImDrawFlags_Closed : ImDrawFlags_None,
        thickness);
}

void DrawContext::PathFill(const FColor& color)
{
    if (!m_DrawList) return;

    m_DrawList->PathFillConvex(color.ToImU32());
}

void DrawContext::PushClipRect(const FVector2& min, const FVector2& max,
                               bool intersectWithCurrent)
{
    if (!m_DrawList) return;

    m_DrawList->PushClipRect(
        min.ToImVec2(),
        max.ToImVec2(),
        intersectWithCurrent);
}

void DrawContext::PopClipRect()
{
    if (!m_DrawList) return;

    m_DrawList->PopClipRect();
}

} // namespace ImWidgetV4
