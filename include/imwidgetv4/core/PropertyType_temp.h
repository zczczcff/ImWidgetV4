#pragma once
#include <string>

namespace ImWidgetV4 {

// 属性类型枚举
enum class PropertyType {
    Int,         // int
    Float,       // float
    Bool,        // bool
    String,      // std::string
    Color,       // ImU32 (RGBA color)
    Vec2,        // ImVec2 (2D vector)
    Struct,      // 嵌套对象 (ReflectableObject*)
    StringArray, // std::vector<std::string>
    Enum         // 可选属性 (枚举类型)
};

// 类型名称转换函数
inline std::string PropertyTypeToString(PropertyType type) {
    switch (type) {
        case PropertyType::Int:         return "Int";
        case PropertyType::Float:       return "Float";
        case PropertyType::Bool:        return "Bool";
        case PropertyType::String:      return "String";
        case PropertyType::Color:       return "Color";
        case PropertyType::Vec2:        return "Vec2";
        case PropertyType::Struct:      return "Struct";
        case PropertyType::StringArray: return "StringArray";
        case PropertyType::Enum:        return "Enum";
        default:                        return "Unknown";
    }
}

inline PropertyType StringToPropertyType(const std::string& str) {
    if (str == "Int")         return PropertyType::Int;
    if (str == "Float")       return PropertyType::Float;
    if (str == "Bool")        return PropertyType::Bool;
    if (str == "String")      return PropertyType::String;
    if (str == "Color")       return PropertyType::Color;
    if (str == "Vec2")        return PropertyType::Vec2;
    if (str == "Struct")      return PropertyType::Struct;
    if (str == "StringArray") return PropertyType::StringArray;
    if (str == "Enum")        return PropertyType::Enum;
    return PropertyType::Int; // 默认值
}

} // namespace ImWidgetV4
