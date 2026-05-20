#include <imwidgetv4/widgets/BoxSlot.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>

namespace ImWidgetV4 {

const Reflection::FTypeDesc& ImBoxSlot::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImBoxSlot, float, &ImBoxSlot::m_FillCoefficient>(
            "ImBoxSlot", "FillCoefficient", Reflection::EPropertyKind::Float, "float", "Fill coefficient for proportional layout")
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImBoxSlot",
        &ImPaddingSlot::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& ImBoxSlotReflectionTypeDesc = ImBoxSlot::StaticTypeDesc();

} // namespace

ImBoxSlot::ImBoxSlot()
    : ImPaddingSlot()
    , m_FillCoefficient(0.0f)
{
}

} // namespace ImWidgetV4
