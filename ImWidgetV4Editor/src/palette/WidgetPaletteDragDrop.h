#pragma once

#include "../serialization/WidgetFactory.h"

#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/DragDrop.h>
#include <imwidgetv4/core/CoreIcon.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FWidgetPaletteEntry {
    std::string Label;
    ImWidgetV4::FText LabelText;
    std::string TypeName;
    ImWidgetV4::ECoreIcon Icon = ImWidgetV4::ECoreIcon::View;
};

class WidgetPalettePayload : public ImWidgetV4::FDragDropPayload {
public:
    static const ImWidgetV4::Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "WidgetPalettePayload"; }
    const ImWidgetV4::Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    std::string WidgetTypeName;
    std::string Label;
};

std::vector<FWidgetPaletteEntry> BuildDefaultWidgetPaletteEntries();

} // namespace ImWidgetV4Editor
