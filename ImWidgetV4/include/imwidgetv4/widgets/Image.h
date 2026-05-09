#pragma once

#include <imwidgetv4/core/Widget.h>
#include <imgui.h>
#include <algorithm>

namespace ImWidgetV4 {

enum class EImageStretchMode {
    KeepAspect,
    Fill
};

struct FImageBrush {
    ImTextureID TextureId = nullptr;
    FVector2 SourceSize {0.0f, 0.0f};
    FVector2 Uv0 {0.0f, 0.0f};
    FVector2 Uv1 {1.0f, 1.0f};
    FColor TintColor = FColor::White;

    bool IsValid() const { return TextureId != nullptr; }
};

class ImImage : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImImage, ImWidget)
    registrar
        .RegisterProperty(PropertyType::Vec2, "DesiredSize", &ImImage::m_DesiredSize, "Desired image size")
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &ImImage::m_BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &ImImage::m_CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Color, "Tint", &ImImage::m_Tint, "Tint color")
        .RegisterOptionalProperty(
            PropertyType::Enum,
            "StretchMode",
            static_cast<void (ImImage::*)(int&)>(&ImImage::SetStretchModeProperty),
            static_cast<int& (ImImage::*)()>(&ImImage::GetStretchModeProperty),
            {"KeepAspect", "Fill"},
            "Image stretch mode");
    END_DECLARE_OBJECT()

public:
    ImImage();
    virtual ~ImImage() = default;

    void SetBrush(const FImageBrush& brush);
    const FImageBrush& GetBrush() const { return m_Brush; }

    void SetTexture(ImTextureID texture, const FVector2& sourceSize = {});
    ImTextureID GetTexture() const { return m_Brush.TextureId; }

    void SetDesiredSize(const FVector2& desiredSize);
    const FVector2& GetDesiredSize() const { return m_DesiredSize; }

    void SetBackgroundColor(const FColor& color);
    const FColor& GetBackgroundColor() const { return m_BackgroundColor; }

    void SetBorderColor(const FColor& color);
    const FColor& GetBorderColor() const { return m_BorderColor; }

    void SetBorderThickness(float thickness);
    float GetBorderThickness() const { return m_BorderThickness; }

    void SetCornerRadius(float radius);
    float GetCornerRadius() const { return m_CornerRadius; }

    void SetTint(const FColor& tint);
    const FColor& GetTint() const { return m_Tint; }

    void SetStretchMode(EImageStretchMode stretchMode);
    EImageStretchMode GetStretchMode() const { return m_StretchMode; }

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;

private:
    void SetStretchModeProperty(int& value)
    {
        value = std::clamp(value, 0, 1);
        SetStretchMode(static_cast<EImageStretchMode>(value));
    }

    int& GetStretchModeProperty()
    {
        m_StretchModeValue = static_cast<int>(m_StretchMode);
        return m_StretchModeValue;
    }

    const FImageBrush& ResolveBrushForPaint() const;
    FGeometry ComputeImageGeometry(const FImageBrush& brush) const;
    FVector2 ResolveImageSourceSize(const FImageBrush& brush) const;

    FImageBrush m_Brush;
    FVector2 m_DesiredSize {0.0f, 0.0f};
    FColor m_BackgroundColor = FColor::FromBytes(34, 40, 49);
    FColor m_BorderColor = FColor::FromBytes(16, 19, 23);
    FColor m_Tint = FColor::White;
    float m_CornerRadius = 6.0f;
    float m_BorderThickness = 1.0f;
    EImageStretchMode m_StretchMode = EImageStretchMode::KeepAspect;
    int m_StretchModeValue = 0;
};

} // namespace ImWidgetV4
