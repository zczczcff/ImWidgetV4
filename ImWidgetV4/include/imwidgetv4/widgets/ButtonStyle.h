#pragma once

#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Types.h>

namespace ImWidgetV4 {

struct FButtonStateStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FButtonStateStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FButtonStateStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FButtonStateStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "TextColor", &FButtonStateStyle::TextColor, "Text color")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FButtonStateStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FButtonStateStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Bool, "HasBorder", &FButtonStateStyle::bHasBorder, "Whether the border is shown");
    END_DECLARE_OBJECT()

public:
    static const Reflection::FTypeDesc& StaticTypeDesc();

    FColor BackgroundColor = FColor::White;
    FColor BorderColor = FColor::Black;
    FColor TextColor = FColor::Black;
    float BorderThickness = 1.0f;
    float CornerRadius = 0.0f;
    bool bHasBorder = false;

    FButtonStateStyle() = default;

    FButtonStateStyle(
        const FColor& bgColor,
        const FColor& borderColor,
        const FColor& textColor,
        float borderThickness = 1.0f,
        float cornerRadius = 0.0f,
        bool hasBorder = false)
        : BackgroundColor(bgColor)
        , BorderColor(borderColor)
        , TextColor(textColor)
        , BorderThickness(borderThickness)
        , CornerRadius(cornerRadius)
        , bHasBorder(hasBorder)
    {
    }
};

struct FButtonStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FButtonStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Struct, "Normal", &FButtonStyle::Normal, "Normal button style")
        .RegisterProperty(PropertyType::Struct, "Hovered", &FButtonStyle::Hovered, "Hovered button style")
        .RegisterProperty(PropertyType::Struct, "Pressed", &FButtonStyle::Pressed, "Pressed button style")
        .RegisterProperty(PropertyType::Struct, "Focused", &FButtonStyle::Focused, "Focused button style")
        .RegisterProperty(PropertyType::Struct, "Disabled", &FButtonStyle::Disabled, "Disabled button style");
    END_DECLARE_OBJECT()

public:
    static const Reflection::FTypeDesc& StaticTypeDesc();

    FButtonStateStyle Normal;
    FButtonStateStyle Hovered;
    FButtonStateStyle Pressed;
    FButtonStateStyle Focused;
    FButtonStateStyle Disabled;

    FButtonStyle()
    {
        Normal = FButtonStateStyle(
            FColor::FromBytes(245, 250, 255, 255),
            FColor::FromBytes(200, 220, 240, 255),
            FColor::FromBytes(50, 50, 50, 255),
            1.0f, 0.0f, false);

        Hovered = FButtonStateStyle(
            FColor::FromBytes(210, 230, 250, 255),
            FColor::FromBytes(120, 170, 220, 255),
            FColor::FromBytes(30, 30, 30, 255),
            1.0f, 0.0f, true);

        Pressed = FButtonStateStyle(
            FColor::FromBytes(190, 220, 245, 255),
            FColor::FromBytes(100, 150, 210, 255),
            FColor::FromBytes(20, 20, 20, 255),
            2.0f, 0.0f, true);

        Focused = FButtonStateStyle(
            FColor::FromBytes(235, 245, 255, 255),
            FColor::FromBytes(0, 120, 215, 255),
            FColor::FromBytes(30, 30, 30, 255),
            2.0f, 0.0f, true);

        Disabled = FButtonStateStyle(
            FColor::FromBytes(240, 240, 240, 255),
            FColor::FromBytes(200, 200, 200, 255),
            FColor::FromBytes(150, 150, 150, 255),
            1.0f, 0.0f, false);
    }

    static FButtonStyle CreatePrimary()
    {
        FButtonStyle style;
        style.Normal = FButtonStateStyle(
            FColor::FromBytes(0, 120, 215, 255),
            FColor::FromBytes(0, 100, 195, 255),
            FColor::FromBytes(255, 255, 255, 255),
            1.0f, 4.0f, false);
        style.Hovered = FButtonStateStyle(
            FColor::FromBytes(0, 140, 235, 255),
            FColor::FromBytes(0, 120, 215, 255),
            FColor::FromBytes(255, 255, 255, 255),
            1.0f, 4.0f, false);
        style.Pressed = FButtonStateStyle(
            FColor::FromBytes(0, 100, 195, 255),
            FColor::FromBytes(0, 80, 175, 255),
            FColor::FromBytes(255, 255, 255, 255),
            2.0f, 4.0f, true);
        style.Focused = FButtonStateStyle(
            FColor::FromBytes(0, 130, 225, 255),
            FColor::FromBytes(255, 255, 255, 255),
            FColor::FromBytes(255, 255, 255, 255),
            2.0f, 4.0f, true);
        style.Disabled = FButtonStateStyle(
            FColor::FromBytes(200, 200, 200, 255),
            FColor::FromBytes(180, 180, 180, 255),
            FColor::FromBytes(150, 150, 150, 255),
            1.0f, 4.0f, false);
        return style;
    }

    static FButtonStyle CreateDanger()
    {
        FButtonStyle style;
        style.Normal = FButtonStateStyle(
            FColor::FromBytes(220, 53, 69, 255),
            FColor::FromBytes(200, 33, 49, 255),
            FColor::FromBytes(255, 255, 255, 255),
            1.0f, 4.0f, false);
        style.Hovered = FButtonStateStyle(
            FColor::FromBytes(240, 73, 89, 255),
            FColor::FromBytes(220, 53, 69, 255),
            FColor::FromBytes(255, 255, 255, 255),
            1.0f, 4.0f, false);
        style.Pressed = FButtonStateStyle(
            FColor::FromBytes(200, 33, 49, 255),
            FColor::FromBytes(180, 13, 29, 255),
            FColor::FromBytes(255, 255, 255, 255),
            2.0f, 4.0f, true);
        style.Focused = FButtonStateStyle(
            FColor::FromBytes(230, 63, 79, 255),
            FColor::FromBytes(255, 255, 255, 255),
            FColor::FromBytes(255, 255, 255, 255),
            2.0f, 4.0f, true);
        style.Disabled = FButtonStateStyle(
            FColor::FromBytes(200, 200, 200, 255),
            FColor::FromBytes(180, 180, 180, 255),
            FColor::FromBytes(150, 150, 150, 255),
            1.0f, 4.0f, false);
        return style;
    }
};

} // namespace ImWidgetV4
