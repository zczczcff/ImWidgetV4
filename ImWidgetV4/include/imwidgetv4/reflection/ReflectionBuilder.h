#pragma once

#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace ImWidgetV4::Reflection {

namespace Detail {

template<typename ClassType, typename ValueType, ValueType ClassType::*Member>
void* GetMemberPtr(void* object)
{
    return &(static_cast<ClassType*>(object)->*Member);
}

template<typename ClassType, typename ValueType, ValueType ClassType::*Member>
const void* GetConstMemberPtr(const void* object)
{
    return &(static_cast<const ClassType*>(object)->*Member);
}

template<typename ValueType>
bool CopyValueImpl(void* destination, const void* source)
{
    if (!destination || !source) {
        return false;
    }

    *static_cast<ValueType*>(destination) = *static_cast<const ValueType*>(source);
    return true;
}

template<typename ClassType, typename ValueType>
constexpr size_t MemberOffset(ValueType ClassType::* member)
{
    return reinterpret_cast<size_t>(
        &(reinterpret_cast<ClassType const volatile*>(0)->*member));
}

} // namespace Detail

template<typename ClassType, typename ValueType, ValueType ClassType::*Member>
FPropertyDesc MakeMemberProperty(
    const char* ownerTypeName,
    const char* name,
    EPropertyKind kind,
    const char* valueTypeName,
    const char* description = "",
    const FTypeDesc* structType = nullptr,
    FEnumOptions enumOptions = {})
{
    FPropertyDesc desc;
    desc.Name = name;
    desc.OwnerTypeName = ownerTypeName;
    desc.ValueTypeName = valueTypeName;
    desc.Description = description;
    desc.Kind = kind;
    desc.Offset = Detail::MemberOffset(Member);
    desc.Size = sizeof(ValueType);
    desc.StructType = structType;
    desc.EnumOptions = enumOptions;
    desc.GetMutablePtr = &Detail::GetMemberPtr<ClassType, ValueType, Member>;
    desc.GetConstPtr = &Detail::GetConstMemberPtr<ClassType, ValueType, Member>;
    desc.CopyValue = &Detail::CopyValueImpl<ValueType>;
    desc.bCustomAccessor = false;
    return desc;
}

} // namespace ImWidgetV4::Reflection
