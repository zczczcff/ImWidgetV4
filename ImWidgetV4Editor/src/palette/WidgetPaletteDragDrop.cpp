#include "WidgetPaletteDragDrop.h"
#include "../editor/WidgetTypeIcon.h"

namespace ImWidgetV4Editor {

std::vector<FWidgetPaletteEntry> BuildDefaultWidgetPaletteEntries()
{
    return {
        {"CanvasPanel", "ImCanvasPanel", TryGetWidgetTypeIcon("ImCanvasPanel").value_or(ImWidgetV4::ECoreIcon::View)},
        {"HorizontalBox", "ImHorizontalBox", TryGetWidgetTypeIcon("ImHorizontalBox").value_or(ImWidgetV4::ECoreIcon::View)},
        {"VerticalBox", "ImVerticalBox", TryGetWidgetTypeIcon("ImVerticalBox").value_or(ImWidgetV4::ECoreIcon::View)},
        {"HorizontalSplitter", "ImHorizontalSplitter", TryGetWidgetTypeIcon("ImHorizontalSplitter").value_or(ImWidgetV4::ECoreIcon::View)},
        {"VerticalSplitter", "ImVerticalSplitter", TryGetWidgetTypeIcon("ImVerticalSplitter").value_or(ImWidgetV4::ECoreIcon::View)},
        {"ScrollBox", "ImScrollBox", TryGetWidgetTypeIcon("ImScrollBox").value_or(ImWidgetV4::ECoreIcon::View)},
        {"TabView", "ImTabView", TryGetWidgetTypeIcon("ImTabView").value_or(ImWidgetV4::ECoreIcon::View)},
        {"ExpandableBox", "ImExpandableBox", TryGetWidgetTypeIcon("ImExpandableBox").value_or(ImWidgetV4::ECoreIcon::View)},
        {"TextBlock", "ImTextBlock", TryGetWidgetTypeIcon("ImTextBlock").value_or(ImWidgetV4::ECoreIcon::View)},
        {"TextList", "ImTextList", TryGetWidgetTypeIcon("ImTextList").value_or(ImWidgetV4::ECoreIcon::View)},
        {"TextOutlineView", "ImTextOutlineView", TryGetWidgetTypeIcon("ImTextOutlineView").value_or(ImWidgetV4::ECoreIcon::View)},
        {"OutlineView", "ImOutlineView", TryGetWidgetTypeIcon("ImOutlineView").value_or(ImWidgetV4::ECoreIcon::View)},
        {"ListView", "ImListView", TryGetWidgetTypeIcon("ImListView").value_or(ImWidgetV4::ECoreIcon::View)},
        {"Button", "ImButton", TryGetWidgetTypeIcon("ImButton").value_or(ImWidgetV4::ECoreIcon::View)},
        {"CheckBox", "ImCheckBox", TryGetWidgetTypeIcon("ImCheckBox").value_or(ImWidgetV4::ECoreIcon::View)},
        {"Switch", "ImSwitch", TryGetWidgetTypeIcon("ImSwitch").value_or(ImWidgetV4::ECoreIcon::View)},
        {"Slider", "ImSlider", TryGetWidgetTypeIcon("ImSlider").value_or(ImWidgetV4::ECoreIcon::View)},
        {"ComboBox", "ImComboBox", TryGetWidgetTypeIcon("ImComboBox").value_or(ImWidgetV4::ECoreIcon::View)},
        {"ColorPicker", "ImColorPicker", TryGetWidgetTypeIcon("ImColorPicker").value_or(ImWidgetV4::ECoreIcon::View)},
        {"EditableText", "ImEditableText", TryGetWidgetTypeIcon("ImEditableText").value_or(ImWidgetV4::ECoreIcon::View)},
        {"Image", "ImImage", TryGetWidgetTypeIcon("ImImage").value_or(ImWidgetV4::ECoreIcon::View)}
    };
}

} // namespace ImWidgetV4Editor
