#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Types.h>
#include <imgui.h>
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

} // namespace

json ReflectableObject::ToJson() const
{
    json result;
    result["Type"] = GetTypeName();

    json properties = json::object();
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
