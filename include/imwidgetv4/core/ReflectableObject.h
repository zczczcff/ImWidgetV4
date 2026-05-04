#pragma once

#include <imwidgetv4/core/PropertyType.h>
#include <imwidgetv4/core/rop/RunTimeObjectProperty.h>
#include <nlohmann/json.hpp>
#include <string>

namespace ImWidgetV4 {

using json = nlohmann::ordered_json;

class ReflectableObject : public ROP::PropertyObject<PropertyType> {
    DECLARE_OBJECT(ReflectableObject)
    END_DECLARE_OBJECT()

public:
    virtual ~ReflectableObject() = default;

    virtual json ToJson() const;
    virtual void FromJson(const json& j);
    virtual std::string GetTypeName() const { return GetClassName(); }

protected:
    using ROPMetaType = ROP::PropertyMeta<
        PropertyType,
        ROPKeyType,
        ROPKeyHash,
        ROPKeyEqual,
        ROPKeyToString,
        ROPStringType,
        ROPErrorCallback>;

    json SerializeProperty(const ROP::PropertyObject<PropertyType>::ROPProperty& prop) const;
    void DeserializeProperty(const std::string& name, const json& value);
    const ROPMetaType* GetPropertyMeta(const ROPProperty& prop) const;

    std::string MakePropertyKey(const std::string& className, const std::string& propName) const;
    std::pair<std::string, std::string> ParsePropertyKey(const std::string& key) const;
};

} // namespace ImWidgetV4
