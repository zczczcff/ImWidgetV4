#pragma once

#include "WidgetPaletteDragDrop.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>

namespace ImWidgetV4Editor {

class WidgetPaletteItemButton : public ImWidgetV4::ImButton {
public:
    explicit WidgetPaletteItemButton(const FWidgetPaletteEntry& entry);

    const FWidgetPaletteEntry& GetEntry() const { return m_Entry; }

    virtual ImWidgetV4::FReply OnInputEvent(const ImWidgetV4::FInputEvent& event) override;
    virtual std::shared_ptr<ImWidgetV4::FDragDropOperation> OnDragDetected(
        const ImWidgetV4::FDragDetectEvent& event) override;

private:
    FWidgetPaletteEntry m_Entry;
};

std::shared_ptr<ImWidgetV4::ImVerticalBox> BuildWidgetPaletteView();

} // namespace ImWidgetV4Editor
