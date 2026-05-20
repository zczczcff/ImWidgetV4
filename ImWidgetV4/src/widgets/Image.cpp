#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imgui_internal.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

namespace {

float ResolveSafeImageRounding(float rounding)
{
    if (rounding <= 0.5f || ImGui::GetCurrentContext() == nullptr) {
        return 0.0f;
    }

    const ImDrawListSharedData* sharedData = ImGui::GetDrawListSharedData();
    if (sharedData == nullptr) {
        return 0.0f;
    }

    if (!std::isfinite(static_cast<double>(sharedData->CircleSegmentMaxError)) ||
        sharedData->CircleSegmentMaxError <= 0.0f) {
        return 0.0f;
    }

    const int radiusIndex = static_cast<int>(std::ceil(rounding));
    if (radiusIndex >= 0 &&
        radiusIndex < static_cast<int>(sizeof(sharedData->CircleSegmentCounts) / sizeof(sharedData->CircleSegmentCounts[0])) &&
        sharedData->CircleSegmentCounts[radiusIndex] == 0) {
        return 0.0f;
    }

    return rounding;
}

} // namespace

const Reflection::FTypeDesc& FImageStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FImageStyle, FColor, &FImageStyle::BackgroundColor>(
            "FImageStyle",
            "BackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Background color"),
        Reflection::MakeMemberProperty<FImageStyle, FColor, &FImageStyle::BorderColor>(
            "FImageStyle",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Border color"),
        Reflection::MakeMemberProperty<FImageStyle, float, &FImageStyle::BorderThickness>(
            "FImageStyle",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Border thickness"),
        Reflection::MakeMemberProperty<FImageStyle, float, &FImageStyle::CornerRadius>(
            "FImageStyle",
            "CornerRadius",
            Reflection::EPropertyKind::Float,
            "float",
            "Corner radius"),
        Reflection::MakeMemberProperty<FImageStyle, FColor, &FImageStyle::Tint>(
            "FImageStyle",
            "Tint",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Tint color")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FImageStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImImage::StaticTypeDesc()
{
    static const char* const stretchModeOptions[] = {"KeepAspect", "Fill"};

    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImImage, FVector2, &ImImage::m_DesiredSize>(
            "ImImage",
            "DesiredSize",
            Reflection::EPropertyKind::Vec2,
            "FVector2",
            "Desired image size"),
        Reflection::MakeObjectAccessorProperty<ImImage, FColor, &ImImage::SetBackgroundColorProperty, &ImImage::GetBackgroundColorProperty>(
            "ImImage",
            "BackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Background color"),
        Reflection::MakeObjectAccessorProperty<ImImage, FColor, &ImImage::SetBorderColorProperty, &ImImage::GetBorderColorProperty>(
            "ImImage",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Border color"),
        Reflection::MakeObjectAccessorProperty<ImImage, float, &ImImage::SetBorderThicknessProperty, &ImImage::GetBorderThicknessProperty>(
            "ImImage",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Border thickness"),
        Reflection::MakeObjectAccessorProperty<ImImage, float, &ImImage::SetCornerRadiusProperty, &ImImage::GetCornerRadiusProperty>(
            "ImImage",
            "CornerRadius",
            Reflection::EPropertyKind::Float,
            "float",
            "Corner radius"),
        Reflection::MakeObjectAccessorProperty<ImImage, FColor, &ImImage::SetTintProperty, &ImImage::GetTintProperty>(
            "ImImage",
            "Tint",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Tint color"),
        Reflection::MakeObjectAccessorProperty<ImImage, int, &ImImage::SetStretchModeProperty, &ImImage::GetStretchModeProperty>(
            "ImImage",
            "StretchMode",
            Reflection::EPropertyKind::Enum,
            "int",
            "Image stretch mode",
            nullptr,
            {stretchModeOptions, sizeof(stretchModeOptions) / sizeof(stretchModeOptions[0])})
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImImage",
        &ImWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FImageStyleReflectionTypeDesc = FImageStyle::StaticTypeDesc();
const Reflection::FTypeDesc& ImImageReflectionTypeDesc = ImImage::StaticTypeDesc();

} // namespace

ImImage::ImImage()
{
    SetHitTestVisible(false);
}

void ImImage::SetStyle(const FImageStyle& style)
{
    m_Style = style;
    m_bHasExplicitStyle = true;
    m_bHasExplicitBackgroundColor = true;
    m_bHasExplicitBorderColor = true;
    m_bHasExplicitBorderThickness = true;
    m_bHasExplicitCornerRadius = true;
    m_bHasExplicitTint = true;
    Invalidate(EInvalidateReason::Paint);
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
    const FImageStyle effectiveStyle = GetEffectiveStyle();
    if (effectiveStyle.BackgroundColor.ToImU32() == color.ToImU32()) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = effectiveStyle;
    }
    m_Style.BackgroundColor = color;
    m_bHasExplicitBackgroundColor = true;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetBorderColor(const FColor& color)
{
    const FImageStyle effectiveStyle = GetEffectiveStyle();
    if (effectiveStyle.BorderColor.ToImU32() == color.ToImU32()) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = effectiveStyle;
    }
    m_Style.BorderColor = color;
    m_bHasExplicitBorderColor = true;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetBorderThickness(float thickness)
{
    const float clampedThickness = std::max(0.0f, thickness);
    if (GetEffectiveStyle().BorderThickness == clampedThickness) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = GetEffectiveStyle();
    }
    m_Style.BorderThickness = clampedThickness;
    m_bHasExplicitBorderThickness = true;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetCornerRadius(float radius)
{
    const float clampedRadius = std::max(0.0f, radius);
    if (GetEffectiveStyle().CornerRadius == clampedRadius) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = GetEffectiveStyle();
    }
    m_Style.CornerRadius = clampedRadius;
    m_bHasExplicitCornerRadius = true;
    Invalidate(EInvalidateReason::Paint);
}

void ImImage::SetTint(const FColor& tint)
{
    const FImageStyle effectiveStyle = GetEffectiveStyle();
    if (effectiveStyle.Tint.ToImU32() == tint.ToImU32()) {
        return;
    }

    if (!m_bHasExplicitStyle) {
        m_Style = effectiveStyle;
    }
    m_Style.Tint = tint;
    m_bHasExplicitTint = true;
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

    const FImageStyle& style = GetEffectiveStyle();

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        style.BackgroundColor,
        style.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        style.BorderColor,
        style.CornerRadius,
        style.BorderThickness);

    const FImageBrush& brush = ResolveBrushForPaint();
    if (!brush.IsValid() || !m_Geometry.IsValid()) {
        return;
    }

    const FGeometry imageGeometry = ComputeImageGeometry(brush);
    if (!imageGeometry.IsValid()) {
        return;
    }

    const FColor finalTint(
        brush.TintColor.R * style.Tint.R,
        brush.TintColor.G * style.Tint.G,
        brush.TintColor.B * style.Tint.B,
        brush.TintColor.A * style.Tint.A);

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

    const float imageRounding = ResolveSafeImageRounding(
        std::max(0.0f, style.CornerRadius - style.BorderThickness));

    if (imageRounding > 0.0f) {
        drawList->AddImageRounded(
            textureId,
            imageGeometry.GetMin().ToImVec2(),
            imageGeometry.GetMax().ToImVec2(),
            brush.Uv0.ToImVec2(),
            brush.Uv1.ToImVec2(),
            finalTint.ToImU32(),
            imageRounding);
        return;
    }

    drawList->AddImage(
        textureId,
        imageGeometry.GetMin().ToImVec2(),
        imageGeometry.GetMax().ToImVec2(),
        brush.Uv0.ToImVec2(),
        brush.Uv1.ToImVec2(),
        finalTint.ToImU32());
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

const FImageStyle& ImImage::GetEffectiveStyle() const
{
    if (m_bHasExplicitStyle) {
        return m_Style;
    }

    if (const ImApplication* application = GetApplication()) {
        m_ResolvedThemeStyle = ResolveImageStyle(application->GetStyleSet());
        if (m_bHasExplicitBackgroundColor) {
            m_ResolvedThemeStyle.BackgroundColor = m_Style.BackgroundColor;
        }
        if (m_bHasExplicitBorderColor) {
            m_ResolvedThemeStyle.BorderColor = m_Style.BorderColor;
        }
        if (m_bHasExplicitBorderThickness) {
            m_ResolvedThemeStyle.BorderThickness = m_Style.BorderThickness;
        }
        if (m_bHasExplicitCornerRadius) {
            m_ResolvedThemeStyle.CornerRadius = m_Style.CornerRadius;
        }
        if (m_bHasExplicitTint) {
            m_ResolvedThemeStyle.Tint = m_Style.Tint;
        }
        return m_ResolvedThemeStyle;
    }

    return m_Style;
}

FColor& ImImage::GetBackgroundColorProperty()
{
    m_BackgroundColorPropertyCache = GetBackgroundColor();
    return m_BackgroundColorPropertyCache;
}

FColor& ImImage::GetBorderColorProperty()
{
    m_BorderColorPropertyCache = GetBorderColor();
    return m_BorderColorPropertyCache;
}

float& ImImage::GetBorderThicknessProperty()
{
    m_BorderThicknessPropertyCache = GetBorderThickness();
    return m_BorderThicknessPropertyCache;
}

float& ImImage::GetCornerRadiusProperty()
{
    m_CornerRadiusPropertyCache = GetCornerRadius();
    return m_CornerRadiusPropertyCache;
}

FColor& ImImage::GetTintProperty()
{
    m_TintPropertyCache = GetTint();
    return m_TintPropertyCache;
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
