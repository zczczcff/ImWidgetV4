#pragma once

#include <imwidgetv4/core/DragDrop.h>

#include <string>

namespace ImWidgetV4Editor {

class WidgetTreeDragDropPayload : public ImWidgetV4::FDragDropPayload {
    DECLARE_OBJECT_WITH_PARENT(WidgetTreeDragDropPayload, ImWidgetV4::FDragDropPayload)
    END_DECLARE_OBJECT()

public:
    std::string WidgetId;
    std::string Label;
};

} // namespace ImWidgetV4Editor
