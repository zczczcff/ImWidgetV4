#pragma once

#include <imwidgetv4/core/Widget.h>
#include <imgui.h>
#include <algorithm>

namespace ImWidgetV4 {

enum class EImageStretchMode {
    KeepAspect,
    Fill
};

struct FImageStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FImageStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor BackgroundColor = FColor::FromBytes(34, 40, 49);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    float BorderThickness = 1.0f;
    float CornerRadius = 6.0f;
    FColor Tint = FColor::White;
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
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImImage"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImImage();
    virtual ~ImImage() = default;

    void SetBrush(const FImageBrush& brush);
    const FImageBrush& GetBrush() const { return m_Brush; }

    void SetTexture(ImTextureID texture, const FVector2& sourceSize = {});
    ImTextureID GetTexture() const { return m_Brush.TextureId; }

    void SetDesiredSize(const FVector2& desiredSize);
    const FVector2& GetDesiredSize() const { return m_DesiredSize; }

    void SetBackgroundColor(const FColor& color);
    const FColor& GetBackgroundColor() const { return GetEffectiveStyle().BackgroundColor; }

    void SetBorderColor(const FColor& color);
    const FColor& GetBorderColor() const { return GetEffectiveStyle().BorderColor; }

    void SetBorderThickness(float thickness);
    float GetBorderThickness() const { return GetEffectiveStyle().BorderThickness; }

    void SetCornerRadius(float radius);
    float GetCornerRadius() const { return GetEffectiveStyle().CornerRadius; }

    void SetTint(const FColor& tint);
    const FColor& GetTint() const { return GetEffectiveStyle().Tint; }

    void SetStyle(const FImageStyle& style);
    const FImageStyle& GetStyle() const { return GetEffectiveStyle(); }

    void SetStretchMode(EImageStretchMode stretchMode);
    EImageStretchMode GetStretchMode() const { return m_StretchMode; }

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;

private:
    const FImageStyle& GetEffectiveStyle() const;
    void SetBackgroundColorProperty(FColor& value) { SetBackgroundColor(value); }
    FColor& GetBackgroundColorProperty();
    void SetBorderColorProperty(FColor& value) { SetBorderColor(value); }
    FColor& GetBorderColorProperty();
    void SetBorderThicknessProperty(float& value) { SetBorderThickness(value); }
    float& GetBorderThicknessProperty();
    void SetCornerRadiusProperty(float& value) { SetCornerRadius(value); }
    float& GetCornerRadiusProperty();
    void SetTintProperty(FColor& value) { SetTint(value); }
    FColor& GetTintProperty();
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
    FImageStyle m_Style;
    mutable FImageStyle m_ResolvedThemeStyle;
    FColor m_BackgroundColorPropertyCache = FColor::FromBytes(34, 40, 49);
    FColor m_BorderColorPropertyCache = FColor::FromBytes(16, 19, 23);
    FColor m_TintPropertyCache = FColor::White;
    float m_CornerRadiusPropertyCache = 6.0f;
    float m_BorderThicknessPropertyCache = 1.0f;
    EImageStretchMode m_StretchMode = EImageStretchMode::KeepAspect;
    int m_StretchModeValue = 0;
    bool m_bHasExplicitStyle = false;
    bool m_bHasExplicitBackgroundColor = false;
    bool m_bHasExplicitBorderColor = false;
    bool m_bHasExplicitBorderThickness = false;
    bool m_bHasExplicitCornerRadius = false;
    bool m_bHasExplicitTint = false;
};

} // namespace ImWidgetV4
