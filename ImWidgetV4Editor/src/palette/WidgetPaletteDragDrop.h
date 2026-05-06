#pragma once

#include "../serialization/WidgetFactory.h"

#include <imwidgetv4/core/DragDrop.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FWidgetPaletteEntry {
    std::string Label;
    std::string TypeName;
};

class WidgetPalettePayload : public ImWidgetV4::FDragDropPayload {
    DECLARE_OBJECT_WITH_PARENT(WidgetPalettePayload, ImWidgetV4::FDragDropPayload)
    END_DECLARE_OBJECT()

public:
    std::string WidgetTypeName;
    std::string Label;
};

std::vector<FWidgetPaletteEntry> BuildDefaultWidgetPaletteEntries();

} // namespace ImWidgetV4Editor
