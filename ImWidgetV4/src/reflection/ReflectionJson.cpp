#include <imwidgetv4/reflection/ReflectionJson.h>

#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Types.h>
#include <imgui.h>
#include <nlohmann/json.hpp>
#include <sstream>

namespace ImWidgetV4::Reflection {

namespace {

std::string MakePropertyKey(const FPropertyDesc& property)
{
    return std::string(property.OwnerTypeName) + "::" + property.Name;
}

std::pair<std::string, std::string> ParsePropertyKey(const std::string& key)
{
    const size_t pos = key.find("::");
    if (pos == std::string::npos) {
        return {"", key};
    }

    return {key.substr(0, pos), key.substr(pos + 2)};
}

bool TypeNameContains(const std::string& typeName, const char* token)
{
    return typeName.find(token) != std::string::npos;
}

FReflectionJson SerializeColorValue(const FPropertyDesc& property, const void* value)
{
    ImU32 color = 0;
    const std::string typeName = property.ValueTypeName ? property.ValueTypeName : "";

    if (TypeNameContains(typeName, "FColor")) {
        color = static_cast<const FColor*>(value)->ToImU32();
    } else {
        color = *static_cast<const ImU32*>(value);
    }

    FReflectionJson colorArray = FReflectionJson::array();
    colorArray.push_back((color >> IM_COL32_R_SHIFT) & 0xFF);
    colorArray.push_back((color >> IM_COL32_G_SHIFT) & 0xFF);
    colorArray.push_back((color >> IM_COL32_B_SHIFT) & 0xFF);
    colorArray.push_back((color >> IM_COL32_A_SHIFT) & 0xFF);
    return colorArray;
}

FReflectionJson SerializeVec2Value(const FPropertyDesc& property, const void* value)
{
    ImVec2 vec;
    const std::string typeName = property.ValueTypeName ? property.ValueTypeName : "";

    if (TypeNameContains(typeName, "FVector2")) {
        vec = static_cast<const FVector2*>(value)->ToImVec2();
    } else {
        vec = *static_cast<const ImVec2*>(value);
    }

    FReflectionJson vecArray = FReflectionJson::array();
    vecArray.push_back(vec.x);
    vecArray.push_back(vec.y);
    return vecArray;
}

FReflectionJson ValueToJson(const FPropertyDesc& property, const FPropertyValue& value)
{
    switch (property.Kind) {
    case EPropertyKind::Int:
        return value.IntValue;
    case EPropertyKind::Float:
        return value.FloatValue;
    case EPropertyKind::Bool:
        return value.BoolValue;
    case EPropertyKind::String:
        return value.StringValue;
    case EPropertyKind::StringArray:
        return value.StringArrayValue;
    case EPropertyKind::Enum:
        if (value.IntValue >= 0 && static_cast<size_t>(value.IntValue) < property.EnumOptions.Count) {
            return property.EnumOptions.Names[value.IntValue];
        }
        return value.IntValue;
    case EPropertyKind::Color:
        return value.ConstStructValue ? SerializeColorValue(property, value.ConstStructValue) : FReflectionJson(nullptr);
    case EPropertyKind::Vec2:
        return value.ConstStructValue ? SerializeVec2Value(property, value.ConstStructValue) : FReflectionJson(nullptr);
    case EPropertyKind::Struct:
        if (value.ConstStructValue && property.StructType && property.GetConstReflectable) {
            const IReflectable* nested = property.GetConstReflectable(value.ConstStructValue);
            if (!nested) {
                return nullptr;
            }
            return ToJson(*nested);
        }
        return nullptr;
    default:
        return nullptr;
    }
}

bool JsonToValue(
    const FPropertyDesc& property,
    const FReflectionJson& json,
    FPropertyValue& outValue,
    std::string* error)
{
    outValue = FPropertyValue {};
    outValue.Kind = property.Kind;

    try {
        switch (property.Kind) {
        case EPropertyKind::Int:
            outValue.IntValue = json.get<int>();
            return true;
        case EPropertyKind::Float:
            outValue.FloatValue = json.get<float>();
            return true;
        case EPropertyKind::Bool:
            outValue.BoolValue = json.get<bool>();
            return true;
        case EPropertyKind::String:
            outValue.StringValue = json.get<std::string>();
            return true;
        case EPropertyKind::StringArray:
            outValue.StringArrayValue = json.get<std::vector<std::string>>();
            return true;
        case EPropertyKind::Enum: {
            if (json.is_number_integer()) {
                outValue.IntValue = json.get<int>();
                return true;
            }

            const std::string option = json.get<std::string>();
            for (size_t index = 0; index < property.EnumOptions.Count; ++index) {
                if (option == property.EnumOptions.Names[index]) {
                    outValue.IntValue = static_cast<int>(index);
                    return true;
                }
            }

            if (error) {
                *error = "Invalid enum option '" + option + "' for property '" + property.Name + "'";
            }
            return false;
        }
        case EPropertyKind::Color:
        case EPropertyKind::Vec2:
        default:
            if (error) {
                *error = "Unsupported reflected JSON property kind";
            }
            return false;
        }
    } catch (const std::exception& e) {
        if (error) {
            *error = e.what();
        }
        return false;
    }
}

bool WriteReflectedColor(
    const FPropertyHandle& handle,
    const FReflectionJson& value,
    std::string* error)
{
    if (!value.is_array() || value.size() != 4) {
        if (error) {
            *error = "Color must be an array of 4 integers";
        }
        return false;
    }

    try {
        const int r = value[0].get<int>();
        const int g = value[1].get<int>();
        const int b = value[2].get<int>();
        const int a = value[3].get<int>();
        const FPropertyDesc* property = handle.GetDesc();
        if (!property) {
            if (error) {
                *error = "Missing color property descriptor";
            }
            return false;
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

        if (!bWritten && error) {
            *error = "Failed to write color property";
        }
        return bWritten;
    } catch (const std::exception& e) {
        if (error) {
            *error = e.what();
        }
        return false;
    }
}

bool WriteReflectedVec2(
    const FPropertyHandle& handle,
    const FReflectionJson& value,
    std::string* error)
{
    if (!value.is_array() || value.size() != 2) {
        if (error) {
            *error = "Vec2 must be an array of 2 floats";
        }
        return false;
    }

    try {
        const float x = value[0].get<float>();
        const float y = value[1].get<float>();
        const FPropertyDesc* property = handle.GetDesc();
        if (!property) {
            if (error) {
                *error = "Missing vec2 property descriptor";
            }
            return false;
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

        if (!bWritten && error) {
            *error = "Failed to write vec2 property";
        }
        return bWritten;
    } catch (const std::exception& e) {
        if (error) {
            *error = e.what();
        }
        return false;
    }
}

} // namespace

FReflectionJson ToJson(const IReflectable& object)
{
    const FTypeDesc& typeDesc = object.GetTypeDesc();

    FReflectionJson result;
    result["Type"] = typeDesc.Name;

    FReflectionJson properties = FReflectionJson::object();
    const auto propertyDescs = CollectProperties(typeDesc);

    for (const FPropertyDesc* property : propertyDescs) {
        if (!property) {
            continue;
        }

        FPropertyHandle handle(const_cast<IReflectable*>(&object), property);
        FPropertyValue value;
        if (!handle.Read(value)) {
            continue;
        }

        properties[MakePropertyKey(*property)] = ValueToJson(*property, value);
    }

    result["Properties"] = properties;
    return result;
}

bool FromJson(IReflectable& object, const FReflectionJson& json, std::string* error)
{
    if (!json.contains("Type") || !json.contains("Properties")) {
        if (error) {
            *error = "Invalid reflected JSON: missing Type or Properties";
        }
        return false;
    }

    const FTypeDesc& typeDesc = object.GetTypeDesc();
    const std::string jsonType = json["Type"].get<std::string>();
    if (jsonType != typeDesc.Name) {
        if (error) {
            *error = "Type mismatch: expected '" + std::string(typeDesc.Name) + "', got '" + jsonType + "'";
        }
        return false;
    }

    const FReflectionJson& properties = json["Properties"];
    if (!properties.is_object()) {
        if (error) {
            *error = "Invalid reflected JSON: Properties must be an object";
        }
        return false;
    }

    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const auto key = ParsePropertyKey(it.key());
        const FPropertyDesc* property = FindProperty(typeDesc, key.second, key.first);
        if (!property) {
            continue;
        }

        FPropertyHandle handle(&object, property);

        if (property->Kind == EPropertyKind::Struct) {
            if (!property->GetReflectable) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': unsupported struct property";
                }
                return false;
            }

            IReflectable* nested = property->GetReflectable(handle.GetMutablePtr());
            if (!nested) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': struct is not reflectable";
                }
                return false;
            }

            std::string nestedError;
            if (!FromJson(*nested, it.value(), &nestedError)) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': " + nestedError;
                }
                return false;
            }
        } else if (property->Kind == EPropertyKind::Color) {
            std::string conversionError;
            if (!WriteReflectedColor(handle, it.value(), &conversionError)) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': " + conversionError;
                }
                return false;
            }
        } else if (property->Kind == EPropertyKind::Vec2) {
            std::string conversionError;
            if (!WriteReflectedVec2(handle, it.value(), &conversionError)) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': " + conversionError;
                }
                return false;
            }
        } else {
            FPropertyValue value;
            std::string conversionError;
            if (!JsonToValue(*property, it.value(), value, &conversionError)) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': " + conversionError;
                }
                return false;
            }

            if (!handle.Write(value)) {
                if (error) {
                    *error = "Failed to write property '" + it.key() + "'";
                }
                return false;
            }
        }
    }

    if (auto* reflectableObject = dynamic_cast<ImWidgetV4::ReflectableObject*>(&object)) {
        reflectableObject->PostDeserializeFromJson();
    }

    return true;
}

} // namespace ImWidgetV4::Reflection

namespace ImWidgetV4 {

json ReflectableObject::ToJson() const
{
    return Reflection::ToJson(*this);
}

void ReflectableObject::FromJson(const json& j)
{
    std::string error;
    if (!Reflection::FromJson(*this, j, &error)) {
        throw std::runtime_error(error);
    }
}

} // namespace ImWidgetV4

#include "../style/StyleSetJson.inl"
#include "../core/LocalizationJson.inl"
