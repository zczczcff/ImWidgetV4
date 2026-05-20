#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ImWidgetV4::Reflection {

enum class EPropertyKind : uint8_t {
    Int,
    Float,
    Bool,
    String,
    Color,
    Vec2,
    Struct,
    StringArray,
    Enum
};

struct FTypeDesc;
class IReflectable;

struct FPropertyValue {
    EPropertyKind Kind = EPropertyKind::Struct;
    int IntValue = 0;
    float FloatValue = 0.0f;
    bool BoolValue = false;
    std::string StringValue;
    std::vector<std::string> StringArrayValue;
    std::array<float, 4> ColorValue {0.0f, 0.0f, 0.0f, 1.0f};
    std::array<float, 2> Vec2Value {0.0f, 0.0f};
    void* StructValue = nullptr;
    const void* ConstStructValue = nullptr;
};

using FGetMutablePtr = void* (*)(void* object);
using FGetConstPtr = const void* (*)(const void* object);
using FCopyValue = bool (*)(void* destination, const void* source);
using FSetValue = bool (*)(void* object, const void* source);
using FReadPropertyValue = bool (*)(const void* source, EPropertyKind kind, FPropertyValue& outValue);
using FWritePropertyValue = bool (*)(void* destination, const FPropertyValue& value);
using FSetObjectPropertyValue = bool (*)(void* object, const FPropertyValue& value);
using FGetReflectable = IReflectable* (*)(void* value);
using FGetConstReflectable = const IReflectable* (*)(const void* value);

struct FEnumOptions {
    const char* const* Names = nullptr;
    size_t Count = 0;
};

struct FPropertyDesc {
    const char* Name = "";
    const char* OwnerTypeName = "";
    const char* ValueTypeName = "";
    const char* Description = "";
    EPropertyKind Kind = EPropertyKind::Struct;
    size_t Offset = 0;
    size_t Size = 0;
    const FTypeDesc* StructType = nullptr;
    FEnumOptions EnumOptions {};
    FGetMutablePtr GetMutablePtr = nullptr;
    FGetConstPtr GetConstPtr = nullptr;
    FCopyValue CopyValue = nullptr;
    FSetValue SetValue = nullptr;
    FReadPropertyValue ReadValue = nullptr;
    FWritePropertyValue WriteValue = nullptr;
    FSetObjectPropertyValue SetObjectValue = nullptr;
    FGetReflectable GetReflectable = nullptr;
    FGetConstReflectable GetConstReflectable = nullptr;
    bool bCustomAccessor = false;
};

struct FTypeDesc {
    const char* Name = "";
    const FTypeDesc* Parent = nullptr;
    const FPropertyDesc* Properties = nullptr;
    size_t PropertyCount = 0;
};

class FPropertyHandle {
public:
    FPropertyHandle() = default;
    FPropertyHandle(void* owner, const FPropertyDesc* desc);

    bool IsValid() const;
    const FPropertyDesc* GetDesc() const;
    const char* GetName() const;
    const char* GetOwnerTypeName() const;
    EPropertyKind GetKind() const;

    void* GetMutablePtr() const;
    const void* GetConstPtr() const;
    bool CopyFrom(const void* source) const;
    bool Read(FPropertyValue& outValue) const;
    bool Write(const FPropertyValue& value) const;

    template<typename T>
    T* GetMutableAs() const
    {
        return static_cast<T*>(GetMutablePtr());
    }

    template<typename T>
    const T* GetConstAs() const
    {
        return static_cast<const T*>(GetConstPtr());
    }

private:
    void* Owner_ = nullptr;
    const FPropertyDesc* Desc_ = nullptr;
};

std::vector<const FPropertyDesc*> CollectProperties(const FTypeDesc& typeDesc);
const FPropertyDesc* FindProperty(
    const FTypeDesc& typeDesc,
    const std::string& name,
    const std::string& ownerTypeName = std::string());

} // namespace ImWidgetV4::Reflection
