#include "WidgetPaletteDragDrop.h"
#include "../editor/EditorLocalization.h"
#include "../editor/WidgetTypeIcon.h"

namespace ImWidgetV4Editor {

std::vector<FWidgetPaletteEntry> BuildDefaultWidgetPaletteEntries()
{
    const auto makeEntry = [](const std::string& label, const std::string& typeName) {
        FWidgetPaletteEntry entry;
        entry.Label = label;
        entry.LabelText = EditorText("Palette." + label, label);
        entry.TypeName = typeName;
        entry.Icon = TryGetWidgetTypeIcon(typeName).value_or(ImWidgetV4::ECoreIcon::View);
        return entry;
    };

    return {
        makeEntry("CanvasPanel", "ImCanvasPanel"),
        makeEntry("HorizontalBox", "ImHorizontalBox"),
        makeEntry("VerticalBox", "ImVerticalBox"),
        makeEntry("HorizontalSplitter", "ImHorizontalSplitter"),
        makeEntry("VerticalSplitter", "ImVerticalSplitter"),
        makeEntry("ScrollBox", "ImScrollBox"),
        makeEntry("TabView", "ImTabView"),
        makeEntry("ExpandableBox", "ImExpandableBox"),
        makeEntry("TextBlock", "ImTextBlock"),
        makeEntry("TextList", "ImTextList"),
        makeEntry("TextOutlineView", "ImTextOutlineView"),
        makeEntry("OutlineView", "ImOutlineView"),
        makeEntry("ListView", "ImListView"),
        makeEntry("Button", "ImButton"),
        makeEntry("CheckBox", "ImCheckBox"),
        makeEntry("Switch", "ImSwitch"),
        makeEntry("Slider", "ImSlider"),
        makeEntry("ComboBox", "ImComboBox"),
        makeEntry("ColorPicker", "ImColorPicker"),
        makeEntry("EditableText", "ImEditableText"),
        makeEntry("Image", "ImImage")
    };
}

} // namespace ImWidgetV4Editor
