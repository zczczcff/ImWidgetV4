#pragma once

#include <imwidgetv4/core/DragDrop.h>

#include <string>

namespace ImWidgetV4Editor {

class WidgetTreeDragDropPayload : public ImWidgetV4::FDragDropPayload {
public:
    static const ImWidgetV4::Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "WidgetTreeDragDropPayload"; }
    const ImWidgetV4::Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    std::string WidgetId;
    std::string Label;
};

} // namespace ImWidgetV4Editor
