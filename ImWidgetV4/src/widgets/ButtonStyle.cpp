#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>

namespace ImWidgetV4 {

const Reflection::FTypeDesc& FButtonStateStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FButtonStateStyle, FColor, &FButtonStateStyle::BackgroundColor>(
            "FButtonStateStyle",
            "BackgroundColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Background color"),
        Reflection::MakeMemberProperty<FButtonStateStyle, FColor, &FButtonStateStyle::BorderColor>(
            "FButtonStateStyle",
            "BorderColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Border color"),
        Reflection::MakeMemberProperty<FButtonStateStyle, FColor, &FButtonStateStyle::TextColor>(
            "FButtonStateStyle",
            "TextColor",
            Reflection::EPropertyKind::Color,
            "FColor",
            "Text color"),
        Reflection::MakeMemberProperty<FButtonStateStyle, float, &FButtonStateStyle::BorderThickness>(
            "FButtonStateStyle",
            "BorderThickness",
            Reflection::EPropertyKind::Float,
            "float",
            "Border thickness"),
        Reflection::MakeMemberProperty<FButtonStateStyle, float, &FButtonStateStyle::CornerRadius>(
            "FButtonStateStyle",
            "CornerRadius",
            Reflection::EPropertyKind::Float,
            "float",
            "Corner radius"),
        Reflection::MakeMemberProperty<FButtonStateStyle, bool, &FButtonStateStyle::bHasBorder>(
            "FButtonStateStyle",
            "HasBorder",
            Reflection::EPropertyKind::Bool,
            "bool",
            "Whether the border is shown")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FButtonStateStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& FButtonStyle::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FButtonStyle, FButtonStateStyle, &FButtonStyle::Normal>(
            "FButtonStyle",
            "Normal",
            Reflection::EPropertyKind::Struct,
            "FButtonStateStyle",
            "Normal button style",
            &FButtonStateStyle::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FButtonStyle, FButtonStateStyle, &FButtonStyle::Hovered>(
            "FButtonStyle",
            "Hovered",
            Reflection::EPropertyKind::Struct,
            "FButtonStateStyle",
            "Hovered button style",
            &FButtonStateStyle::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FButtonStyle, FButtonStateStyle, &FButtonStyle::Pressed>(
            "FButtonStyle",
            "Pressed",
            Reflection::EPropertyKind::Struct,
            "FButtonStateStyle",
            "Pressed button style",
            &FButtonStateStyle::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FButtonStyle, FButtonStateStyle, &FButtonStyle::Focused>(
            "FButtonStyle",
            "Focused",
            Reflection::EPropertyKind::Struct,
            "FButtonStateStyle",
            "Focused button style",
            &FButtonStateStyle::StaticTypeDesc()),
        Reflection::MakeMemberProperty<FButtonStyle, FButtonStateStyle, &FButtonStyle::Disabled>(
            "FButtonStyle",
            "Disabled",
            Reflection::EPropertyKind::Struct,
            "FButtonStateStyle",
            "Disabled button style",
            &FButtonStateStyle::StaticTypeDesc())
    };

    static const Reflection::FTypeDesc typeDesc {
        "FButtonStyle",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& FButtonStateStyleReflectionTypeDesc = FButtonStateStyle::StaticTypeDesc();
const Reflection::FTypeDesc& FButtonStyleReflectionTypeDesc = FButtonStyle::StaticTypeDesc();

} // namespace

} // namespace ImWidgetV4
