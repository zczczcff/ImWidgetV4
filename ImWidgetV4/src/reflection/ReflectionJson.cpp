#include <imwidgetv4/reflection/ReflectionJson.h>

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
    case EPropertyKind::Struct:
        if (value.ConstStructValue && property.StructType && property.GetConstReflectable) {
            const IReflectable* nested = property.GetConstReflectable(value.ConstStructValue);
            if (!nested) {
                return nullptr;
            }
            return ToJson(*nested);
        }
        return nullptr;
    case EPropertyKind::Color:
    case EPropertyKind::Vec2:
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

        if (property->Kind == EPropertyKind::Struct) {
            FPropertyHandle handle(&object, property);
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
        } else {
            FPropertyValue value;
            std::string conversionError;
            if (!JsonToValue(*property, it.value(), value, &conversionError)) {
                if (error) {
                    *error = "Failed to deserialize property '" + it.key() + "': " + conversionError;
                }
                return false;
            }

            FPropertyHandle handle(&object, property);
            if (!handle.Write(value)) {
                if (error) {
                    *error = "Failed to write property '" + it.key() + "'";
                }
                return false;
            }
        }
    }

    return true;
}

} // namespace ImWidgetV4::Reflection
