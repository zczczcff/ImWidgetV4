#pragma once

#include <imwidgetv4/reflection/ReflectionTypes.h>

namespace ImWidgetV4::Reflection {

class IReflectable {
public:
    virtual ~IReflectable() = default;
    virtual const FTypeDesc& GetTypeDesc() const = 0;
};

} // namespace ImWidgetV4::Reflection
