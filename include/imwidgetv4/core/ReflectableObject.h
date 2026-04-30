#pragma once
#include <imwidgetv4/core/rop/RunTimeObjectProperty.h>
#include <imwidgetv4/core/PropertyType.h>
#include <nlohmann/json.hpp>
#include <string>

namespace ImWidgetV4 {

using json = nlohmann::ordered_json;

class ReflectableObject : public ROP::PropertyObject<PropertyType> {
public:
    virtual ~ReflectableObject() = default;

    // 序列化为 JSON
    virtual json ToJson() const;

    // 从 JSON 反序列化
    virtual void FromJson(const json& j);

    // 获取类型名称（子类必须实现）
    virtual std::string GetTypeName() const = 0;

protected:
    // 序列化单个属性
    json SerializeProperty(const ROP::PropertyObject<PropertyType>::ROPProperty& prop) const;

    // 反序列化单个属性
    void DeserializeProperty(const std::string& name, const json& value);

    // 处理同名属性冲突
    std::string MakePropertyKey(const std::string& className, const std::string& propName) const;
    std::pair<std::string, std::string> ParsePropertyKey(const std::string& key) const;
};

} // namespace ImWidgetV4
