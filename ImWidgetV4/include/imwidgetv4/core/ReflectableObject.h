#pragma once

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
    FReflectedOptionalProperty() = default;
    FReflectedOptionalProperty(ReflectableObject* owner, const Reflection::FPropertyDesc* desc);

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
};

class ReflectableObject : public Reflection::IReflectable {
public:
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
};

} // namespace ImWidgetV4
