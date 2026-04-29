#include <imwidgetv4/core/ReflectableObject.h>
#include <stdexcept>
#include <sstream>

namespace ImWidgetV4 {

// ToJson 方法：序列化为 JSON
json ReflectableObject::ToJson() const {
    json result;

    // 添加类型信息
    result["Type"] = GetTypeName();

    // 创建 Properties 对象
    json properties = json::object();

    // 获取所有属性（包括继承的）
    auto allProps = GetAllPropertiesOrdered();

    // 遍历所有属性并序列化
    for (const auto& prop : allProps) {
        std::string propName = prop.GetName();
        std::string className = prop.GetClassName();

        // 生成属性键（处理同名属性）
        std::string key = MakePropertyKey(className, propName);

        // 序列化属性值
        properties[key] = SerializeProperty(prop);
    }

    result["Properties"] = properties;

    return result;
}

// SerializeProperty 方法：序列化单个属性
json ReflectableObject::SerializeProperty(const ROPProperty& prop) const {
    PropertyType type = prop.GetType();

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
            // ImU32 转换为 [r, g, b, a] 数组
            ImU32 color = prop.GetValue<ImU32>();
            json colorArray = json::array();
            colorArray.push_back((color >> IM_COL32_R_SHIFT) & 0xFF);
            colorArray.push_back((color >> IM_COL32_G_SHIFT) & 0xFF);
            colorArray.push_back((color >> IM_COL32_B_SHIFT) & 0xFF);
            colorArray.push_back((color >> IM_COL32_A_SHIFT) & 0xFF);
            return colorArray;
        }

        case PropertyType::Vec2: {
            // ImVec2 转换为 [x, y] 数组
            ImVec2 vec = prop.GetValue<ImVec2>();
            json vecArray = json::array();
            vecArray.push_back(vec.x);
            vecArray.push_back(vec.y);
            return vecArray;
        }

        case PropertyType::Enum: {
            // 使用 OptionalProperty 获取枚举字符串
            auto optProp = ToOptionalProperty(prop);
            return optProp.GetOptionString();
        }

        case PropertyType::Struct: {
            // 递归序列化嵌套对象
            ReflectableObject* nestedObj = prop.GetValue<ReflectableObject*>();
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

// FromJson 方法：从 JSON 反序列化
void ReflectableObject::FromJson(const json& j) {
    // 验证 JSON 格式
    if (!j.contains("Type") || !j.contains("Properties")) {
        throw std::runtime_error("Invalid JSON format: missing 'Type' or 'Properties' field");
    }

    // 验证类型名称
    std::string jsonType = j["Type"].get<std::string>();
    if (jsonType != GetTypeName()) {
        std::ostringstream oss;
        oss << "Type mismatch: expected '" << GetTypeName()
            << "', got '" << jsonType << "'";
        throw std::runtime_error(oss.str());
    }

    // 获取 Properties 对象
    const json& properties = j["Properties"];
    if (!properties.is_object()) {
        throw std::runtime_error("Invalid JSON format: 'Properties' must be an object");
    }

    // 遍历所有属性并反序列化
    for (auto it = properties.begin(); it != properties.end(); ++it) {
        const std::string& key = it.key();
        const json& value = it.value();

        DeserializeProperty(key, value);
    }
}

// DeserializeProperty 方法：反序列化单个属性
void ReflectableObject::DeserializeProperty(const std::string& name, const json& value) {
    // 解析属性键
    std::pair<std::string, std::string> keyPair = ParsePropertyKey(name);
    std::string className = keyPair.first;
    std::string propName = keyPair.second;

    // 获取属性
    ROPProperty prop;
    if (!className.empty()) {
        // 指定了类名，精确查找
        prop = GetProperty(propName, className);
    } else {
        // 未指定类名，查找第一个匹配的属性
        prop = GetProperty(propName);
    }

    if (!prop.IsValid()) {
        // 属性不存在，跳过（允许 JSON 包含额外字段）
        return;
    }

    PropertyType type = prop.GetType();

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
                // [r, g, b, a] 数组转换为 ImU32
                if (!value.is_array() || value.size() != 4) {
                    throw std::runtime_error("Color must be an array of 4 integers");
                }
                int r = value[0].get<int>();
                int g = value[1].get<int>();
                int b = value[2].get<int>();
                int a = value[3].get<int>();
                ImU32 color = IM_COL32(r, g, b, a);
                prop.SetValue<ImU32>(color);
                break;
            }

            case PropertyType::Vec2: {
                // [x, y] 数组转换为 ImVec2
                if (!value.is_array() || value.size() != 2) {
                    throw std::runtime_error("Vec2 must be an array of 2 floats");
                }
                float x = value[0].get<float>();
                float y = value[1].get<float>();
                ImVec2 vec(x, y);
                prop.SetValue<ImVec2>(vec);
                break;
            }

            case PropertyType::Enum: {
                // 使用 OptionalProperty 设置枚举值
                auto optProp = ToOptionalProperty(prop);
                std::string optionStr = value.get<std::string>();
                if (!optProp.SetOptionByString(optionStr)) {
                    std::ostringstream oss;
                    oss << "Invalid enum value '" << optionStr
                        << "' for property '" << propName << "'";
                    throw std::runtime_error(oss.str());
                }
                break;
            }

            case PropertyType::Struct: {
                // 递归反序列化嵌套对象
                ReflectableObject* nestedObj = prop.GetValue<ReflectableObject*>();
                if (nestedObj && !value.is_null()) {
                    nestedObj->FromJson(value);
                }
                break;
            }

            case PropertyType::StringArray:
                prop.SetValue<std::vector<std::string>>(
                    value.get<std::vector<std::string>>());
                break;

            default:
                // 未知类型，跳过
                break;
        }
    } catch (const std::exception& e) {
        std::ostringstream oss;
        oss << "Failed to deserialize property '" << name << "': " << e.what();
        throw std::runtime_error(oss.str());
    }
}

// MakePropertyKey 方法：生成属性键
std::string ReflectableObject::MakePropertyKey(
    const std::string& className,
    const std::string& propName) const {
    return className + "::" + propName;
}

// ParsePropertyKey 方法：解析属性键
std::pair<std::string, std::string> ReflectableObject::ParsePropertyKey(
    const std::string& key) const {
    size_t pos = key.find("::");
    if (pos != std::string::npos) {
        // 包含 "::" 分隔符
        std::string className = key.substr(0, pos);
        std::string propName = key.substr(pos + 2);
        return {className, propName};
    } else {
        // 不包含分隔符，只有属性名
        return {"", key};
    }
}

} // namespace ImWidgetV4
