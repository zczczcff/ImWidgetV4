#pragma once
#include <imwidgetv4/core/rop/RunTimeObjectProperty.h>
#include <imwidgetv4/core/PropertyType.h>
#include <nlohmann/json.hpp>
#include <imgui.h>
#include <string>
#include <utility>

namespace ImWidgetV4 {

// 使用 nlohmann::ordered_json 保持属性顺序
using json = nlohmann::ordered_json;

/**
 * @brief 可反射对象基类
 *
 * 提供完整的 JSON 序列化/反序列化功能，支持：
 * - 基本类型：Int, Float, Bool, String
 * - 图形类型：Color (ImU32), Vec2 (ImVec2)
 * - 复杂类型：Struct (嵌套对象), StringArray, Enum
 * - 继承链：自动处理父类属性
 * - 同名属性：使用 "ClassName::PropertyName" 格式区分
 */
class ReflectableObject : public ROP::PropertyObject<PropertyType> {
public:
    virtual ~ReflectableObject() = default;

    /**
     * @brief 序列化为 JSON
     * @return JSON 对象，包含 "Type" 和 "Properties" 字段
     */
    virtual json ToJson() const;

    /**
     * @brief 从 JSON 反序列化
     * @param j JSON 对象，必须包含 "Type" 和 "Properties" 字段
     * @throws std::runtime_error 如果 JSON 格式无效或类型不匹配
     */
    virtual void FromJson(const json& j);

    /**
     * @brief 获取类型名称
     * @return 类型名称字符串
     */
    virtual std::string GetTypeName() const = 0;

protected:
    /**
     * @brief 序列化单个属性
     * @param prop 属性对象
     * @return 属性的 JSON 表示
     */
    json SerializeProperty(const ROPProperty& prop) const;

    /**
     * @brief 反序列化单个属性
     * @param name 属性名称
     * @param value 属性的 JSON 值
     */
    void DeserializeProperty(const std::string& name, const json& value);

    /**
     * @brief 生成属性键（处理同名属性）
     * @param className 类名
     * @param propName 属性名
     * @return "ClassName::PropertyName" 格式的键
     */
    std::string MakePropertyKey(const std::string& className, const std::string& propName) const;

    /**
     * @brief 解析属性键
     * @param key 属性键（可能包含 "::" 分隔符）
     * @return pair<className, propName>
     */
    std::pair<std::string, std::string> ParsePropertyKey(const std::string& key) const;
};

} // namespace ImWidgetV4
