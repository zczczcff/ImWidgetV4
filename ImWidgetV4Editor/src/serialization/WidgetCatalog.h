#pragma once

#include "WidgetFactory.h"

#include <imwidgetv4/reflection/ReflectionTypes.h>

#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FWidgetPropertyInfo {
    std::string OwnerTypeName;
    std::string Name;
    std::string ValueTypeName;
    std::string Description;
    ImWidgetV4::Reflection::EPropertyKind Kind = ImWidgetV4::Reflection::EPropertyKind::Struct;
    std::vector<std::string> EnumOptions;
    bool bIsInherited = false;
};

struct FWidgetTypeInfo {
    std::string TypeName;
    bool bSupported = false;
    std::vector<FWidgetPropertyInfo> Properties;
};

class WidgetCatalog {
public:
    static WidgetCatalog& Get();

    std::vector<std::string> ListWidgetTypes() const;
    bool TryDescribeWidgetType(const std::string& typeName, FWidgetTypeInfo& outInfo) const;

private:
    WidgetCatalog() = default;
};

} // namespace ImWidgetV4Editor
