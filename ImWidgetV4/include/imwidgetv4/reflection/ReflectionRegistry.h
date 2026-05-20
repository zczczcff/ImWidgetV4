#pragma once

#include <imwidgetv4/reflection/ReflectionTypes.h>
#include <string>
#include <unordered_map>
#include <vector>

namespace ImWidgetV4::Reflection {

class FReflectionRegistry {
public:
    static FReflectionRegistry& Get();

    bool RegisterType(const FTypeDesc* typeDesc);
    const FTypeDesc* FindType(const std::string& typeName) const;
    std::vector<const FTypeDesc*> GetTypes() const;
    void Clear();

private:
    std::unordered_map<std::string, const FTypeDesc*> Types_;
};

class FAutoTypeRegistration {
public:
    explicit FAutoTypeRegistration(const FTypeDesc* typeDesc);
};

} // namespace ImWidgetV4::Reflection
