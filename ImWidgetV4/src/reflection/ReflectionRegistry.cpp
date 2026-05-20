#include <imwidgetv4/reflection/ReflectionRegistry.h>

namespace ImWidgetV4::Reflection {

FReflectionRegistry& FReflectionRegistry::Get()
{
    static FReflectionRegistry registry;
    return registry;
}

bool FReflectionRegistry::RegisterType(const FTypeDesc* typeDesc)
{
    if (!typeDesc || !typeDesc->Name || typeDesc->Name[0] == '\0') {
        return false;
    }

    Types_[typeDesc->Name] = typeDesc;
    return true;
}

const FTypeDesc* FReflectionRegistry::FindType(const std::string& typeName) const
{
    const auto it = Types_.find(typeName);
    return it != Types_.end() ? it->second : nullptr;
}

std::vector<const FTypeDesc*> FReflectionRegistry::GetTypes() const
{
    std::vector<const FTypeDesc*> result;
    result.reserve(Types_.size());

    for (const auto& pair : Types_) {
        result.push_back(pair.second);
    }

    return result;
}

void FReflectionRegistry::Clear()
{
    Types_.clear();
}

FAutoTypeRegistration::FAutoTypeRegistration(const FTypeDesc* typeDesc)
{
    FReflectionRegistry::Get().RegisterType(typeDesc);
}

} // namespace ImWidgetV4::Reflection
