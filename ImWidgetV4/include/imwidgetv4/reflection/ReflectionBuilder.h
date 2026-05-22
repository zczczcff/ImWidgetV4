#pragma once

#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <imwidgetv4/reflection/Reflectable.h>
#include <cstddef>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

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

template<typename ClassType, typename ValueType, ValueType& (ClassType::*Getter)()>
void* GetAccessorPtr(void* object)
{
    return &(static_cast<ClassType*>(object)->*Getter)();
}

template<typename ClassType, typename ValueType, ValueType& (ClassType::*Getter)()>
const void* GetConstAccessorPtr(const void* object)
{
    return &(const_cast<ClassType*>(static_cast<const ClassType*>(object))->*Getter)();
}

template<typename ClassType, typename ValueType, void (ClassType::*Setter)(ValueType&)>
bool CopyThroughAccessor(void* object, const void* source)
{
    if (!object || !source) {
        return false;
    }

    ValueType value = *static_cast<const ValueType*>(source);
    (static_cast<ClassType*>(object)->*Setter)(value);
    return true;
}

template<typename ValueType>
bool ReadTypedValue(const void* source, EPropertyKind kind, FPropertyValue& outValue)
{
    if (!source) {
        return false;
    }

    outValue = FPropertyValue {};
    outValue.Kind = kind;
    const ValueType& value = *static_cast<const ValueType*>(source);

    if constexpr (std::is_same_v<ValueType, int>) {
        outValue.IntValue = value;
        return true;
    } else if constexpr (std::is_same_v<ValueType, float>) {
        outValue.FloatValue = value;
        return true;
    } else if constexpr (std::is_same_v<ValueType, bool>) {
        outValue.BoolValue = value;
        return true;
    } else if constexpr (std::is_same_v<ValueType, std::string>) {
        outValue.StringValue = value;
        return true;
    } else if constexpr (std::is_same_v<ValueType, std::vector<std::string>>) {
        outValue.StringArrayValue = value;
        return true;
    } else {
        outValue.ConstStructValue = source;
        return kind == EPropertyKind::Struct || kind == EPropertyKind::Color || kind == EPropertyKind::Vec2;
    }
}

template<typename ValueType>
bool WriteTypedValue(void* destination, const FPropertyValue& value)
{
    if (!destination) {
        return false;
    }

    ValueType* typedDestination = static_cast<ValueType*>(destination);

    if constexpr (std::is_same_v<ValueType, int>) {
        *typedDestination = value.IntValue;
        return true;
    } else if constexpr (std::is_same_v<ValueType, float>) {
        *typedDestination = value.FloatValue;
        return true;
    } else if constexpr (std::is_same_v<ValueType, bool>) {
        *typedDestination = value.BoolValue;
        return true;
    } else if constexpr (std::is_same_v<ValueType, std::string>) {
        *typedDestination = value.StringValue;
        return true;
    } else if constexpr (std::is_same_v<ValueType, std::vector<std::string>>) {
        *typedDestination = value.StringArrayValue;
        return true;
    } else {
        return false;
    }
}

template<typename ClassType, typename ValueType, void (ClassType::*Setter)(ValueType&)>
bool SetObjectTypedValue(void* object, const FPropertyValue& value)
{
    if (!object) {
        return false;
    }

    ValueType converted {};
    if (!WriteTypedValue<ValueType>(&converted, value)) {
        return false;
    }

    (static_cast<ClassType*>(object)->*Setter)(converted);
    return true;
}

template<typename ValueType>
IReflectable* CastReflectable(void* value)
{
    if constexpr (std::is_pointer_v<ValueType>) {
        using PointeeType = std::remove_pointer_t<ValueType>;
        if constexpr (std::is_base_of_v<IReflectable, PointeeType>) {
            if (!value) {
                return nullptr;
            }
            return static_cast<IReflectable*>(*static_cast<ValueType*>(value));
        } else {
            return nullptr;
        }
    } else if constexpr (std::is_base_of_v<IReflectable, ValueType>) {
        return static_cast<IReflectable*>(static_cast<ValueType*>(value));
    } else {
        return nullptr;
    }
}

template<typename ValueType>
const IReflectable* CastConstReflectable(const void* value)
{
    if constexpr (std::is_pointer_v<ValueType>) {
        using PointeeType = std::remove_pointer_t<ValueType>;
        if constexpr (std::is_base_of_v<IReflectable, PointeeType>) {
            if (!value) {
                return nullptr;
            }
            return static_cast<const IReflectable*>(*static_cast<ValueType const*>(value));
        } else {
            return nullptr;
        }
    } else if constexpr (std::is_base_of_v<IReflectable, ValueType>) {
        return static_cast<const IReflectable*>(static_cast<const ValueType*>(value));
    } else {
        return nullptr;
    }
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
    FEnumOptions enumOptions = {},
    EPropertyFlags flags = EPropertyFlags::None)
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
    desc.Flags = flags;
    desc.GetMutablePtr = &Detail::GetMemberPtr<ClassType, ValueType, Member>;
    desc.GetConstPtr = &Detail::GetConstMemberPtr<ClassType, ValueType, Member>;
    desc.CopyValue = &Detail::CopyValueImpl<ValueType>;
    desc.SetValue = nullptr;
    desc.ReadValue = &Detail::ReadTypedValue<ValueType>;
    desc.WriteValue = &Detail::WriteTypedValue<ValueType>;
    desc.SetObjectValue = nullptr;
    desc.GetReflectable = &Detail::CastReflectable<ValueType>;
    desc.GetConstReflectable = &Detail::CastConstReflectable<ValueType>;
    desc.bCustomAccessor = false;
    return desc;
}

template<typename ClassType, typename ValueType, void (ClassType::*Setter)(ValueType&), ValueType& (ClassType::*Getter)()>
FPropertyDesc MakeAccessorProperty(
    const char* ownerTypeName,
    const char* name,
    EPropertyKind kind,
    const char* valueTypeName,
    const char* description = "",
    const FTypeDesc* structType = nullptr,
    FEnumOptions enumOptions = {},
    EPropertyFlags flags = EPropertyFlags::None)
{
    FPropertyDesc desc;
    desc.Name = name;
    desc.OwnerTypeName = ownerTypeName;
    desc.ValueTypeName = valueTypeName;
    desc.Description = description;
    desc.Kind = kind;
    desc.Offset = 0;
    desc.Size = sizeof(ValueType);
    desc.StructType = structType;
    desc.EnumOptions = enumOptions;
    desc.Flags = flags;
    desc.GetMutablePtr = &Detail::GetAccessorPtr<ClassType, ValueType, Getter>;
    desc.GetConstPtr = &Detail::GetConstAccessorPtr<ClassType, ValueType, Getter>;
    desc.CopyValue = nullptr;
    desc.SetValue = nullptr;
    desc.ReadValue = &Detail::ReadTypedValue<ValueType>;
    desc.WriteValue = nullptr;
    desc.SetObjectValue = nullptr;
    desc.GetReflectable = &Detail::CastReflectable<ValueType>;
    desc.GetConstReflectable = &Detail::CastConstReflectable<ValueType>;
    desc.bCustomAccessor = true;
    return desc;
}

template<typename ClassType, typename ValueType, void (ClassType::*Setter)(ValueType&), ValueType& (ClassType::*Getter)()>
FPropertyDesc MakeObjectAccessorProperty(
    const char* ownerTypeName,
    const char* name,
    EPropertyKind kind,
    const char* valueTypeName,
    const char* description = "",
    const FTypeDesc* structType = nullptr,
    FEnumOptions enumOptions = {},
    EPropertyFlags flags = EPropertyFlags::None)
{
    FPropertyDesc desc = MakeAccessorProperty<ClassType, ValueType, Setter, Getter>(
        ownerTypeName,
        name,
        kind,
        valueTypeName,
        description,
        structType,
        enumOptions,
        flags);
    desc.SetValue = &Detail::CopyThroughAccessor<ClassType, ValueType, Setter>;
    desc.SetObjectValue = &Detail::SetObjectTypedValue<ClassType, ValueType, Setter>;
    return desc;
}

} // namespace ImWidgetV4::Reflection
