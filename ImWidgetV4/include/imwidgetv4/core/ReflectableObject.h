#pragma once

#include <imwidgetv4/core/PropertyType.h>
#include <imwidgetv4/core/rop/RunTimeObjectProperty.h>
#include <imwidgetv4/reflection/Reflectable.h>
#include <nlohmann/json_fwd.hpp>
#include <string>
#include <vector>

#ifdef GetClassName
#undef GetClassName
#endif

namespace ImWidgetV4 {

using json = nlohmann::ordered_json;

class ReflectableObject;

class FReflectedOptionalProperty {
public:
    using LegacyOptionalProperty = ROP::PropertyObject<PropertyType>::ROPOptionalProperty;

    FReflectedOptionalProperty() = default;
    FReflectedOptionalProperty(ReflectableObject* owner, const Reflection::FPropertyDesc* desc);
    explicit FReflectedOptionalProperty(const LegacyOptionalProperty& legacyProperty);

    bool IsValid() const;
    std::string GetOptionString() const;
    const std::vector<std::string>& GetOptionList() const;
    bool SetOptionByString(const std::string& option);
    bool SetOptionByIndex(int index);
    size_t GetOptionCount() const;

private:
    ReflectableObject* Owner_ = nullptr;
    const Reflection::FPropertyDesc* Desc_ = nullptr;
    std::vector<std::string> Options_;
    LegacyOptionalProperty LegacyProperty_;
};

class ReflectableObject : public ROP::PropertyObject<PropertyType>, public Reflection::IReflectable {
public:
    ROPStringType GetClassName() const override { return "ReflectableObject"; }
    const ROPPropertyDataType& GetPropertyData() const override { return GetPropertyDataStatic(); }
    static ROPPropertyDataType& GetPropertyDataStatic();
    static bool StaticInitializeProperties();

    virtual ~ReflectableObject() = default;

    virtual json ToJson() const;
    virtual void FromJson(const json& j);
    virtual std::string GetTypeName() const { return "ReflectableObject"; }
    virtual const Reflection::FTypeDesc& GetTypeDesc() const override;
    const Reflection::FTypeDesc* FindReflectionTypeDesc() const;
    bool HasProperty(const std::string& name) const;
    bool HasProperty(const std::string& name, const std::string& className) const;
    FReflectedOptionalProperty GetPropertyAsOptional(const std::string& name);
    FReflectedOptionalProperty GetPropertyAsOptional(const std::string& name, const std::string& className);

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
