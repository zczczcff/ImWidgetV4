#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/reflection/Reflectable.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <sstream>
#include <stdexcept>

namespace ImWidgetV4 {

namespace {

bool TypeNameContains(const std::string& typeName, const char* token)
{
    return typeName.find(token) != std::string::npos;
}

bool IsPointerTypeName(const std::string& typeName)
{
    return !typeName.empty() && typeName.back() == '*';
}

std::string MakeReflectionPropertyKey(const Reflection::FPropertyDesc& property)
{
    return std::string(property.OwnerTypeName) + "::" + property.Name;
}

std::pair<std::string, std::string> ParseReflectionPropertyKey(const std::string& key)
{
    const size_t pos = key.find("::");
    if (pos == std::string::npos) {
        return {"", key};
    }

    return {key.substr(0, pos), key.substr(pos + 2)};
}

json SerializeColorValue(const Reflection::FPropertyDesc& property, const void* value)
{
    ImU32 color = 0;
    const std::string typeName = property.ValueTypeName ? property.ValueTypeName : "";

    if (TypeNameContains(typeName, "FColor")) {
        color = static_cast<const FColor*>(value)->ToImU32();
    } else {
        color = *static_cast<const ImU32*>(value);
    }

    json colorArray = json::array();
    colorArray.push_back((color >> IM_COL32_R_SHIFT) & 0xFF);
    colorArray.push_back((color >> IM_COL32_G_SHIFT) & 0xFF);
    colorArray.push_back((color >> IM_COL32_B_SHIFT) & 0xFF);
    colorArray.push_back((color >> IM_COL32_A_SHIFT) & 0xFF);
    return colorArray;
}

json SerializeVec2Value(const Reflection::FPropertyDesc& property, const void* value)
{
    ImVec2 vec;
    const std::string typeName = property.ValueTypeName ? property.ValueTypeName : "";

    if (TypeNameContains(typeName, "FVector2")) {
        vec = static_cast<const FVector2*>(value)->ToImVec2();
    } else {
        vec = *static_cast<const ImVec2*>(value);
    }

    json vecArray = json::array();
    vecArray.push_back(vec.x);
    vecArray.push_back(vec.y);
    return vecArray;
}

json SerializeReflectedProperty(const Reflection::FPropertyDesc& property, const Reflection::FPropertyValue& value)
{
    switch (property.Kind) {
    case Reflection::EPropertyKind::Int:
        return value.IntValue;
    case Reflection::EPropertyKind::Float:
        return value.FloatValue;
    case Reflection::EPropertyKind::Bool:
        return value.BoolValue;
    case Reflection::EPropertyKind::String:
        return value.StringValue;
    case Reflection::EPropertyKind::StringArray:
        return value.StringArrayValue;
    case Reflection::EPropertyKind::Enum:
        if (value.IntValue >= 0 && static_cast<size_t>(value.IntValue) < property.EnumOptions.Count) {
            return property.EnumOptions.Names[value.IntValue];
        }
        return value.IntValue;
    case Reflection::EPropertyKind::Color:
        return value.ConstStructValue ? SerializeColorValue(property, value.ConstStructValue) : json(nullptr);
    case Reflection::EPropertyKind::Vec2:
        return value.ConstStructValue ? SerializeVec2Value(property, value.ConstStructValue) : json(nullptr);
    case Reflection::EPropertyKind::Struct:
        if (value.ConstStructValue && property.GetConstReflectable) {
            const Reflection::IReflectable* nested = property.GetConstReflectable(value.ConstStructValue);
            if (const ReflectableObject* nestedObject = dynamic_cast<const ReflectableObject*>(nested)) {
                return nestedObject->ToJson();
            }
        }
        return json(nullptr);
    default:
        return json(nullptr);
    }
}

void WriteReflectedColor(const Reflection::FPropertyHandle& handle, const json& value)
{
    if (!value.is_array() || value.size() != 4) {
        throw std::runtime_error("Color must be an array of 4 integers");
    }

    const int r = value[0].get<int>();
    const int g = value[1].get<int>();
    const int b = value[2].get<int>();
    const int a = value[3].get<int>();
    const Reflection::FPropertyDesc* property = handle.GetDesc();
    if (!property) {
        throw std::runtime_error("Missing color property descriptor");
    }

    const std::string typeName = property->ValueTypeName ? property->ValueTypeName : "";

    bool bWritten = false;
    if (TypeNameContains(typeName, "FColor")) {
        FColor color = FColor::FromBytes(r, g, b, a);
        bWritten = handle.CopyFrom(&color);
    } else {
        ImU32 color = IM_COL32(r, g, b, a);
        bWritten = handle.CopyFrom(&color);
    }

    if (!bWritten) {
        throw std::runtime_error("Failed to write color property");
    }
}

void WriteReflectedVec2(const Reflection::FPropertyHandle& handle, const json& value)
{
    if (!value.is_array() || value.size() != 2) {
        throw std::runtime_error("Vec2 must be an array of 2 floats");
    }

    const float x = value[0].get<float>();
    const float y = value[1].get<float>();
    const Reflection::FPropertyDesc* property = handle.GetDesc();
    if (!property) {
        throw std::runtime_error("Missing vec2 property descriptor");
    }

    const std::string typeName = property->ValueTypeName ? property->ValueTypeName : "";

    bool bWritten = false;
    if (TypeNameContains(typeName, "FVector2")) {
        FVector2 vec(x, y);
        bWritten = handle.CopyFrom(&vec);
    } else {
        ImVec2 vec(x, y);
        bWritten = handle.CopyFrom(&vec);
    }

    if (!bWritten) {
        throw std::runtime_error("Failed to write vec2 property");
    }
}

Reflection::FPropertyValue JsonToReflectedValue(const Reflection::FPropertyDesc& property, const json& value)
{
    Reflection::FPropertyValue result;
    result.Kind = property.Kind;

    switch (property.Kind) {
    case Reflection::EPropertyKind::Int:
        result.IntValue = value.get<int>();
        return result;
    case Reflection::EPropertyKind::Float:
        result.FloatValue = value.get<float>();
        return result;
    case Reflection::EPropertyKind::Bool:
        result.BoolValue = value.get<bool>();
        return result;
    case Reflection::EPropertyKind::String:
        result.StringValue = value.get<std::string>();
        return result;
    case Reflection::EPropertyKind::StringArray:
        result.StringArrayValue = value.get<std::vector<std::string>>();
        return result;
    case Reflection::EPropertyKind::Enum:
        if (value.is_number_integer()) {
            result.IntValue = value.get<int>();
            return result;
        }

        {
            const std::string option = value.get<std::string>();
            for (size_t index = 0; index < property.EnumOptions.Count; ++index) {
                if (option == property.EnumOptions.Names[index]) {
                    result.IntValue = static_cast<int>(index);
                    return result;
                }
            }

            throw std::runtime_error("Invalid enum value '" + option + "'");
        }
    default:
        return result;
    }
}

} // namespace

FReflectedOptionalProperty::FReflectedOptionalProperty(ReflectableObject* owner, const Reflection::FPropertyDesc* desc)
    : Owner_(owner)
    , Desc_(desc)
{
    if (!desc || desc->Kind != Reflection::EPropertyKind::Enum) {
        return;
    }

    Options_.reserve(desc->EnumOptions.Count);
    for (size_t index = 0; index < desc->EnumOptions.Count; ++index) {
        Options_.emplace_back(desc->EnumOptions.Names[index]);
    }
}

FReflectedOptionalProperty::FReflectedOptionalProperty(const LegacyOptionalProperty& legacyProperty)
    : LegacyProperty_(legacyProperty)
{
}

bool FReflectedOptionalProperty::IsValid() const
{
    return (Owner_ && Desc_ && Desc_->Kind == Reflection::EPropertyKind::Enum) || LegacyProperty_.IsValid();
}

std::string FReflectedOptionalProperty::GetOptionString() const
{
    if (LegacyProperty_.IsValid()) {
        return LegacyProperty_.GetOptionString();
    }

    if (!Owner_ || !Desc_) {
        return std::string();
    }

    Reflection::FPropertyHandle handle(Owner_, Desc_);
    Reflection::FPropertyValue value;
    if (!handle.Read(value) || value.IntValue < 0 || static_cast<size_t>(value.IntValue) >= Options_.size()) {
        return std::string();
    }

    return Options_[value.IntValue];
}

const std::vector<std::string>& FReflectedOptionalProperty::GetOptionList() const
{
    if (LegacyProperty_.IsValid()) {
        return LegacyProperty_.GetOptionList();
    }

    return Options_;
}

bool FReflectedOptionalProperty::SetOptionByString(const std::string& option)
{
    if (LegacyProperty_.IsValid()) {
        return LegacyProperty_.SetOptionByString(option);
    }

    for (size_t index = 0; index < Options_.size(); ++index) {
        if (Options_[index] == option) {
            return SetOptionByIndex(static_cast<int>(index));
        }
    }

    return false;
}

bool FReflectedOptionalProperty::SetOptionByIndex(int index)
{
    if (LegacyProperty_.IsValid()) {
        return LegacyProperty_.SetOptionByIndex(index);
    }

    if (!Owner_ || !Desc_ || index < 0 || static_cast<size_t>(index) >= Options_.size()) {
        return false;
    }

    Reflection::FPropertyHandle handle(Owner_, Desc_);
    Reflection::FPropertyValue value;
    value.Kind = Reflection::EPropertyKind::Enum;
    value.IntValue = index;
    return handle.Write(value);
}

size_t FReflectedOptionalProperty::GetOptionCount() const
{
    if (LegacyProperty_.IsValid()) {
        return LegacyProperty_.GetOptionCount();
    }

    return Options_.size();
}

const Reflection::FTypeDesc& ReflectableObject::GetTypeDesc() const
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        return *typeDesc;
    }

    static const Reflection::FTypeDesc fallbackTypeDesc {
        "ReflectableObject",
        nullptr,
        nullptr,
        0
    };
    return fallbackTypeDesc;
}

ReflectableObject::ROPPropertyDataType& ReflectableObject::GetPropertyDataStatic()
{
    static ROPPropertyDataType propertyData;
    return propertyData;
}

bool ReflectableObject::StaticInitializeProperties()
{
    ROPPropertyDataType& propertyData = GetPropertyDataStatic();
    if (!propertyData.initialized) {
        propertyData.initialized = true;
    }
    return true;
}

const Reflection::FTypeDesc* ReflectableObject::FindReflectionTypeDesc() const
{
    return Reflection::FReflectionRegistry::Get().FindType(GetTypeName());
}

bool ReflectableObject::HasProperty(const std::string& name) const
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (Reflection::FindProperty(*typeDesc, name)) {
            return true;
        }
    }

    return ROP::PropertyObject<PropertyType>::HasProperty(name);
}

bool ReflectableObject::HasProperty(const std::string& name, const std::string& className) const
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (Reflection::FindProperty(*typeDesc, name, className)) {
            return true;
        }
    }

    return ROP::PropertyObject<PropertyType>::HasProperty(name, className);
}

FReflectedOptionalProperty ReflectableObject::GetPropertyAsOptional(const std::string& name)
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (const Reflection::FPropertyDesc* property = Reflection::FindProperty(*typeDesc, name)) {
            return FReflectedOptionalProperty(this, property);
        }
    }

    return FReflectedOptionalProperty(ROP::PropertyObject<PropertyType>::GetPropertyAsOptional(name));
}

FReflectedOptionalProperty ReflectableObject::GetPropertyAsOptional(const std::string& name, const std::string& className)
{
    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        if (const Reflection::FPropertyDesc* property = Reflection::FindProperty(*typeDesc, name, className)) {
            return FReflectedOptionalProperty(this, property);
        }
    }

    return FReflectedOptionalProperty(ROP::PropertyObject<PropertyType>::GetPropertyAsOptional(name, className));
}

json ReflectableObject::ToJson() const
{
    json result;
    result["Type"] = GetTypeName();

    json properties = json::object();

    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        const auto reflectedProperties = Reflection::CollectProperties(*typeDesc);
        for (const Reflection::FPropertyDesc* property : reflectedProperties) {
            if (!property) {
                continue;
            }

            Reflection::FPropertyHandle handle(const_cast<ReflectableObject*>(this), property);
            Reflection::FPropertyValue value;
            if (!handle.Read(value)) {
                continue;
            }

            properties[MakeReflectionPropertyKey(*property)] = SerializeReflectedProperty(*property, value);
        }

        result["Properties"] = properties;
        return result;
    }

    auto allProps = GetAllPropertiesOrdered();

    for (const auto& prop : allProps) {
        const std::string propName = prop.GetName();
        const std::string className = prop.GetClassName();
        const std::string key = MakePropertyKey(className, propName);
        properties[key] = SerializeProperty(prop);
    }

    result["Properties"] = properties;
    return result;
}

const ReflectableObject::ROPMetaType* ReflectableObject::GetPropertyMeta(const ROPProperty& prop) const
{
    return static_cast<const ROPMetaType*>(prop.GetMetaPtr());
}

json ReflectableObject::SerializeProperty(const ROPProperty& prop) const
{
    const PropertyType type = prop.GetType();
    const ROPMetaType* meta = GetPropertyMeta(prop);
    const std::string typeName = meta ? meta->typeName : std::string();

    switch (type) {
    case PropertyType::Int:
        return prop.GetValue<int>();

    case PropertyType::Float:
        return prop.GetValue<float>();

    case PropertyType::Bool:
        return prop.GetValue<bool>();

    case PropertyType::String:
        return prop.GetValue<std::string>();

    case PropertyType::Color: {
        const ImU32 color = TypeNameContains(typeName, "FColor")
            ? prop.GetValue<FColor>().ToImU32()
            : prop.GetValue<ImU32>();

        json colorArray = json::array();
        colorArray.push_back((color >> IM_COL32_R_SHIFT) & 0xFF);
        colorArray.push_back((color >> IM_COL32_G_SHIFT) & 0xFF);
        colorArray.push_back((color >> IM_COL32_B_SHIFT) & 0xFF);
        colorArray.push_back((color >> IM_COL32_A_SHIFT) & 0xFF);
        return colorArray;
    }

    case PropertyType::Vec2: {
        const ImVec2 vec = TypeNameContains(typeName, "FVector2")
            ? prop.GetValue<FVector2>().ToImVec2()
            : prop.GetValue<ImVec2>();

        json vecArray = json::array();
        vecArray.push_back(vec.x);
        vecArray.push_back(vec.y);
        return vecArray;
    }

    case PropertyType::Enum: {
        auto optProp = ToOptionalProperty(prop);
        return optProp.GetOptionString();
    }

    case PropertyType::Struct: {
        ReflectableObject* nestedObj = IsPointerTypeName(typeName)
            ? prop.GetValue<ReflectableObject*>()
            : const_cast<ReflectableObject*>(prop.GetConstPointer<ReflectableObject>());
        if (nestedObj) {
            return nestedObj->ToJson();
        }
        return json(nullptr);
    }

    case PropertyType::StringArray:
        return prop.GetValue<std::vector<std::string>>();

    default:
        return json(nullptr);
    }
}

void ReflectableObject::FromJson(const json& j)
{
    if (!j.contains("Type") || !j.contains("Properties")) {
        throw std::runtime_error("Invalid JSON format: missing 'Type' or 'Properties' field");
    }

    const std::string jsonType = j["Type"].get<std::string>();
    if (jsonType != GetTypeName()) {
        std::ostringstream oss;
        oss << "Type mismatch: expected '" << GetTypeName() << "', got '" << jsonType << "'";
        throw std::runtime_error(oss.str());
    }

    const json& properties = j["Properties"];
    if (!properties.is_object()) {
        throw std::runtime_error("Invalid JSON format: 'Properties' must be an object");
    }

    if (const Reflection::FTypeDesc* typeDesc = FindReflectionTypeDesc()) {
        for (auto it = properties.begin(); it != properties.end(); ++it) {
            const auto keyPair = ParseReflectionPropertyKey(it.key());
            const Reflection::FPropertyDesc* property = Reflection::FindProperty(*typeDesc, keyPair.second, keyPair.first);
            if (!property) {
                continue;
            }

            Reflection::FPropertyHandle handle(this, property);

            try {
                switch (property->Kind) {
                case Reflection::EPropertyKind::Color:
                    WriteReflectedColor(handle, it.value());
                    break;
                case Reflection::EPropertyKind::Vec2:
                    WriteReflectedVec2(handle, it.value());
                    break;
                case Reflection::EPropertyKind::Struct:
                    if (property->GetReflectable && !it.value().is_null()) {
                        Reflection::IReflectable* nested = property->GetReflectable(handle.GetMutablePtr());
                        if (ReflectableObject* nestedObject = dynamic_cast<ReflectableObject*>(nested)) {
                            nestedObject->FromJson(it.value());
                        }
                    }
                    break;
                default: {
                    const Reflection::FPropertyValue value = JsonToReflectedValue(*property, it.value());
                    if (!handle.Write(value)) {
                        throw std::runtime_error("Failed to write reflected property");
                    }
                    break;
                }
                }
            } catch (const std::exception& e) {
                std::ostringstream oss;
                oss << "Failed to deserialize property '" << it.key() << "': " << e.what();
                throw std::runtime_error(oss.str());
            }
        }

        return;
    }

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        DeserializeProperty(it.key(), it.value());
    }
}

void ReflectableObject::DeserializeProperty(const std::string& name, const json& value)
{
    const auto keyPair = ParsePropertyKey(name);
    const std::string& className = keyPair.first;
    const std::string& propName = keyPair.second;

    ROPProperty prop = !className.empty()
        ? GetProperty(propName, className)
        : GetProperty(propName);

    if (!prop.IsValid()) {
        return;
    }

    const PropertyType type = prop.GetType();
    const ROPMetaType* meta = GetPropertyMeta(prop);
    const std::string typeName = meta ? meta->typeName : std::string();

    try {
        switch (type) {
        case PropertyType::Int:
            prop.SetValue<int>(value.get<int>());
            break;

        case PropertyType::Float:
            prop.SetValue<float>(value.get<float>());
            break;

        case PropertyType::Bool:
            prop.SetValue<bool>(value.get<bool>());
            break;

        case PropertyType::String:
            prop.SetValue<std::string>(value.get<std::string>());
            break;

        case PropertyType::Color: {
            if (!value.is_array() || value.size() != 4) {
                throw std::runtime_error("Color must be an array of 4 integers");
            }

            const int r = value[0].get<int>();
            const int g = value[1].get<int>();
            const int b = value[2].get<int>();
            const int a = value[3].get<int>();

            if (TypeNameContains(typeName, "FColor")) {
                prop.SetValue<FColor>(FColor::FromBytes(r, g, b, a));
            } else {
                prop.SetValue<ImU32>(IM_COL32(r, g, b, a));
            }
            break;
        }

        case PropertyType::Vec2: {
            if (!value.is_array() || value.size() != 2) {
                throw std::runtime_error("Vec2 must be an array of 2 floats");
            }

            const float x = value[0].get<float>();
            const float y = value[1].get<float>();

            if (TypeNameContains(typeName, "FVector2")) {
                prop.SetValue<FVector2>(FVector2(x, y));
            } else {
                prop.SetValue<ImVec2>(ImVec2(x, y));
            }
            break;
        }

        case PropertyType::Enum: {
            auto optProp = ToOptionalProperty(prop);
            const std::string optionStr = value.get<std::string>();
            if (!optProp.SetOptionByString(optionStr)) {
                std::ostringstream oss;
                oss << "Invalid enum value '" << optionStr << "' for property '" << propName << "'";
                throw std::runtime_error(oss.str());
            }
            break;
        }

        case PropertyType::Struct: {
            ReflectableObject* nestedObj = IsPointerTypeName(typeName)
                ? prop.GetValue<ReflectableObject*>()
                : prop.GetPointer<ReflectableObject>();
            if (nestedObj && !value.is_null()) {
                nestedObj->FromJson(value);
            }
            break;
        }

        case PropertyType::StringArray:
            prop.SetValue<std::vector<std::string>>(value.get<std::vector<std::string>>());
            break;

        default:
            break;
        }
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Failed to deserialize property '" << name << "': " << e.what();
        throw std::runtime_error(oss.str());
    }
}

std::string ReflectableObject::MakePropertyKey(const std::string& className, const std::string& propName) const
{
    return className + "::" + propName;
}

std::pair<std::string, std::string> ReflectableObject::ParsePropertyKey(const std::string& key) const
{
    const size_t pos = key.find("::");
    if (pos != std::string::npos) {
        return {key.substr(0, pos), key.substr(pos + 2)};
    }

    return {"", key};
}

} // namespace ImWidgetV4
