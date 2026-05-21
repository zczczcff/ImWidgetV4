#include "WidgetCatalog.h"

#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>

#include <algorithm>
#include <memory>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4::Reflection;

namespace {

std::string PropertyKindToString(EPropertyKind kind)
{
    switch (kind) {
    case EPropertyKind::Int: return "Int";
    case EPropertyKind::Float: return "Float";
    case EPropertyKind::Bool: return "Bool";
    case EPropertyKind::String: return "String";
    case EPropertyKind::Color: return "Color";
    case EPropertyKind::Vec2: return "Vec2";
    case EPropertyKind::Struct: return "Struct";
    case EPropertyKind::StringArray: return "StringArray";
    case EPropertyKind::Enum: return "Enum";
    }
    return "Struct";
}

} // namespace

WidgetCatalog& WidgetCatalog::Get()
{
    static WidgetCatalog instance;
    return instance;
}

std::vector<std::string> WidgetCatalog::ListWidgetTypes() const
{
    return WidgetFactory::Get().GetRegisteredWidgetTypes();
}

bool WidgetCatalog::TryDescribeWidgetType(const std::string& typeName, FWidgetTypeInfo& outInfo) const
{
    const auto widget = WidgetFactory::Get().CreateWidget(typeName);
    if (!widget) {
        return false;
    }

    const auto reflectable = std::dynamic_pointer_cast<ReflectableObject>(widget);
    if (!reflectable) {
        return false;
    }

    const FTypeDesc& typeDesc = reflectable->GetTypeDesc();
    outInfo = {};
    outInfo.TypeName = typeDesc.Name ? typeDesc.Name : typeName;
    outInfo.bSupported = true;

    const auto properties = CollectProperties(typeDesc);
    outInfo.Properties.reserve(properties.size());
    for (const FPropertyDesc* property : properties) {
        if (!property) {
            continue;
        }

        FWidgetPropertyInfo info;
        info.OwnerTypeName = property->OwnerTypeName ? property->OwnerTypeName : "";
        info.Name = property->Name ? property->Name : "";
        info.ValueTypeName = property->ValueTypeName ? property->ValueTypeName : "";
        info.Description = property->Description ? property->Description : "";
        info.Kind = property->Kind;
        info.bIsInherited = info.OwnerTypeName != outInfo.TypeName;
        if (property->EnumOptions.Names && property->EnumOptions.Count > 0) {
            info.EnumOptions.reserve(property->EnumOptions.Count);
            for (size_t index = 0; index < property->EnumOptions.Count; ++index) {
                info.EnumOptions.emplace_back(property->EnumOptions.Names[index]);
            }
        }
        outInfo.Properties.push_back(std::move(info));
    }

    std::sort(outInfo.Properties.begin(), outInfo.Properties.end(), [](const FWidgetPropertyInfo& left, const FWidgetPropertyInfo& right) {
        if (left.OwnerTypeName == right.OwnerTypeName) {
            return left.Name < right.Name;
        }
        return left.OwnerTypeName < right.OwnerTypeName;
    });

    return true;
}

} // namespace ImWidgetV4Editor
