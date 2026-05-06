#include "WidgetPaletteDragDrop.h"

namespace ImWidgetV4Editor {

std::vector<FWidgetPaletteEntry> BuildDefaultWidgetPaletteEntries()
{
    return {
        {"CanvasPanel", "ImCanvasPanel"},
        {"HorizontalBox", "ImHorizontalBox"},
        {"VerticalBox", "ImVerticalBox"},
        {"ScrollBox", "ImScrollBox"},
        {"TabView", "ImTabView"},
        {"TextBlock", "ImTextBlock"},
        {"Button", "ImButton"},
        {"CheckBox", "ImCheckBox"},
        {"Switch", "ImSwitch"},
        {"Slider", "ImSlider"},
        {"EditableText", "ImEditableText"},
        {"Image", "ImImage"}
    };
}

} // namespace ImWidgetV4Editor
