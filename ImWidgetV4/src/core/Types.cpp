#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>

namespace ImWidgetV4 {

namespace {

const Reflection::FTypeDesc& GetFMarginReflectionTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<FMargin, float, &FMargin::Left>(
            "FMargin",
            "Left",
            Reflection::EPropertyKind::Float,
            "float",
            "Left margin"),
        Reflection::MakeMemberProperty<FMargin, float, &FMargin::Right>(
            "FMargin",
            "Right",
            Reflection::EPropertyKind::Float,
            "float",
            "Right margin"),
        Reflection::MakeMemberProperty<FMargin, float, &FMargin::Top>(
            "FMargin",
            "Top",
            Reflection::EPropertyKind::Float,
            "float",
            "Top margin"),
        Reflection::MakeMemberProperty<FMargin, float, &FMargin::Bottom>(
            "FMargin",
            "Bottom",
            Reflection::EPropertyKind::Float,
            "float",
            "Bottom margin")
    };

    static const Reflection::FTypeDesc typeDesc {
        "FMargin",
        nullptr,
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& FMarginReflectionTypeDesc = GetFMarginReflectionTypeDesc();

} // namespace

// FVector2 静态常量
const FVector2 FVector2::Zero = FVector2(0.0f, 0.0f);
const FVector2 FVector2::One = FVector2(1.0f, 1.0f);
const FVector2 FVector2::UnitX = FVector2(1.0f, 0.0f);
const FVector2 FVector2::UnitY = FVector2(0.0f, 1.0f);

// FColor 静态常量
const FColor FColor::White = FColor(1.0f, 1.0f, 1.0f, 1.0f);
const FColor FColor::Black = FColor(0.0f, 0.0f, 0.0f, 1.0f);
const FColor FColor::Red = FColor(1.0f, 0.0f, 0.0f, 1.0f);
const FColor FColor::Green = FColor(0.0f, 1.0f, 0.0f, 1.0f);
const FColor FColor::Blue = FColor(0.0f, 0.0f, 1.0f, 1.0f);
const FColor FColor::Yellow = FColor(1.0f, 1.0f, 0.0f, 1.0f);
const FColor FColor::Cyan = FColor(0.0f, 1.0f, 1.0f, 1.0f);
const FColor FColor::Magenta = FColor(1.0f, 0.0f, 1.0f, 1.0f);
const FColor FColor::Transparent = FColor(0.0f, 0.0f, 0.0f, 0.0f);
const FColor FColor::Gray = FColor(0.5f, 0.5f, 0.5f, 1.0f);

} // namespace ImWidgetV4
