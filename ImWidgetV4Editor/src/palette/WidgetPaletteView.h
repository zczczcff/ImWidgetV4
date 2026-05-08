#pragma once

#include "WidgetPaletteDragDrop.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>

namespace ImWidgetV4Editor {

class WidgetPaletteItemButton : public ImWidgetV4::ImButton {
public:
    explicit WidgetPaletteItemButton(const FWidgetPaletteEntry& entry);

    const FWidgetPaletteEntry& GetEntry() const { return m_Entry; }

    virtual void Paint(const ImWidgetV4::FPaintContext& paintContext) override;
    virtual ImWidgetV4::FReply OnInputEvent(const ImWidgetV4::FInputEvent& event) override;
    virtual std::shared_ptr<ImWidgetV4::FDragDropOperation> OnDragDetected(
        const ImWidgetV4::FDragDetectEvent& event) override;

private:
    void EnsureVisualContent();

    FWidgetPaletteEntry m_Entry;
    std::shared_ptr<ImWidgetV4::ImImage> m_IconImage;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_LabelText;
    bool m_bVisualContentInitialized = false;
};

std::shared_ptr<ImWidgetV4::ImVerticalBox> BuildWidgetPaletteView();

} // namespace ImWidgetV4Editor
