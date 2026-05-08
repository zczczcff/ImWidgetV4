#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>

namespace ImWidgetV4 {

ImImage::ImImage()
{
    SetHitTestVisible(false);
}

void ImImage::SetBrush(const FImageBrush& brush)
{
    const bool bChanged =
        m_Brush.TextureId != brush.TextureId ||
        m_Brush.SourceSize != brush.SourceSize ||
        m_Brush.Uv0 != brush.Uv0 ||
        m_Brush.Uv1 != brush.Uv1 ||
        m_Brush.TintColor.ToImU32() != brush.TintColor.ToImU32();
    if (!bChanged) {
        return;
    }

    m_Brush = brush;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImImage::SetTexture(ImTextureID texture, const FVector2& sourceSize)
{
    FImageBrush brush = m_Brush;
    brush.TextureId = texture;
    if (sourceSize.X > 0.0f && sourceSize.Y > 0.0f) {
        brush.SourceSize = sourceSize;
    } else if (texture == nullptr) {
        brush.SourceSize = FVector2(0.0f, 0.0f);
    }
    SetBrush(brush);
}

void ImImage::SetDesiredSize(const FVector2& desiredSize)
{
    if (m_DesiredSize == desiredSize) {
        return;
    }

    m_DesiredSize = desiredSize;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImImage::SetBackgroundColor(const FColor& color)
{
    if (m_BackgroundColor.ToImU32() == color.ToImU32()) {
        return;
    }

    m_BackgroundColor = color;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetCornerRadius(float radius)
{
    const float clampedRadius = std::max(0.0f, radius);
    if (m_CornerRadius == clampedRadius) {
        return;
    }

    m_CornerRadius = clampedRadius;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetTint(const FColor& tint)
{
    if (m_Tint.ToImU32() == tint.ToImU32()) {
        return;
    }

    m_Tint = tint;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetStretchMode(EImageStretchMode stretchMode)
{
    if (m_StretchMode == stretchMode) {
        return;
    }

    m_StretchMode = stretchMode;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImImage::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        m_BackgroundColor,
        m_CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        m_BorderColor,
        m_CornerRadius,
        m_BorderThickness);

    const FImageBrush& brush = ResolveBrushForPaint();
    if (!brush.IsValid() || !m_Geometry.IsValid()) {
        return;
    }

    const FGeometry imageGeometry = ComputeImageGeometry(brush);
    if (!imageGeometry.IsValid()) {
        return;
    }

    const FColor finalTint(
        brush.TintColor.R * m_Tint.R,
        brush.TintColor.G * m_Tint.G,
        brush.TintColor.B * m_Tint.B,
        brush.TintColor.A * m_Tint.A);

    ImDrawList* drawList = paintContext.DrawContext_.GetImDrawList();
    if (drawList == nullptr) {
        return;
    }

    ImTextureID textureId = brush.TextureId;
    if (m_Application != nullptr) {
        textureId = m_Application->ResolveTextureForPaint(textureId);
    }
    if (textureId == nullptr) {
        return;
    }

    drawList->AddImageRounded(
        textureId,
        imageGeometry.GetMin().ToImVec2(),
        imageGeometry.GetMax().ToImVec2(),
        brush.Uv0.ToImVec2(),
        brush.Uv1.ToImVec2(),
        finalTint.ToImU32(),
        std::max(0.0f, m_CornerRadius - m_BorderThickness));
}

FVector2 ImImage::GetMinSize() const
{
    if (m_DesiredSize.X > 0.0f && m_DesiredSize.Y > 0.0f) {
        return m_DesiredSize;
    }

    const FImageBrush& brush = ResolveBrushForPaint();
    const FVector2 sourceSize = ResolveImageSourceSize(brush);
    if (sourceSize.X > 0.0f && sourceSize.Y > 0.0f) {
        return sourceSize;
    }

    return FVector2(96.0f, 72.0f);
}

const FImageBrush& ImImage::ResolveBrushForPaint() const
{
    if (m_Brush.IsValid()) {
        return m_Brush;
    }

    if (m_Application != nullptr) {
        return m_Application->GetDefaultImagePlaceholderBrush();
    }

    return m_Brush;
}

FGeometry ImImage::ComputeImageGeometry(const FImageBrush& brush) const
{
    const FVector2 availableSize = m_Geometry.Size;
    if (m_StretchMode == EImageStretchMode::Fill) {
        return m_Geometry;
    }

    const FVector2 sourceSize = ResolveImageSourceSize(brush);
    if (sourceSize.X <= 0.0f || sourceSize.Y <= 0.0f ||
        availableSize.X <= 0.0f || availableSize.Y <= 0.0f) {
        return m_Geometry;
    }

    const float scale = std::min(availableSize.X / sourceSize.X, availableSize.Y / sourceSize.Y);
    const FVector2 scaledSize(
        std::max(0.0f, sourceSize.X * scale),
        std::max(0.0f, sourceSize.Y * scale));
    const FVector2 offset(
        std::max(0.0f, (availableSize.X - scaledSize.X) * 0.5f),
        std::max(0.0f, (availableSize.Y - scaledSize.Y) * 0.5f));
    return FGeometry(m_Geometry.Position + offset, scaledSize);
}

FVector2 ImImage::ResolveImageSourceSize(const FImageBrush& brush) const
{
    if (brush.SourceSize.X > 0.0f && brush.SourceSize.Y > 0.0f) {
        return brush.SourceSize;
    }

    return m_DesiredSize;
}

} // namespace ImWidgetV4
