#include "WidgetPaletteDragDrop.h"
#include "../editor/EditorLocalization.h"
#include "../editor/WidgetTypeIcon.h"
#include "../tree/WidgetTreeDragDrop.h"

#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>

namespace ImWidgetV4Editor {

const ImWidgetV4::Reflection::FTypeDesc& WidgetPalettePayload::StaticTypeDesc()
{
    static const ImWidgetV4::Reflection::FPropertyDesc properties[] = {
        ImWidgetV4::Reflection::MakeMemberProperty<WidgetPalettePayload, std::string, &WidgetPalettePayload::WidgetTypeName>(
            "WidgetPalettePayload",
            "WidgetTypeName",
            ImWidgetV4::Reflection::EPropertyKind::String,
            "std::string",
            "Widget type name"),
        ImWidgetV4::Reflection::MakeMemberProperty<WidgetPalettePayload, std::string, &WidgetPalettePayload::Label>(
            "WidgetPalettePayload",
            "Label",
            ImWidgetV4::Reflection::EPropertyKind::String,
            "std::string",
            "Label")
    };

    static const ImWidgetV4::Reflection::FTypeDesc typeDesc {
        "WidgetPalettePayload",
        &ImWidgetV4::FDragDropPayload::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const ImWidgetV4::Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const ImWidgetV4::Reflection::FTypeDesc& WidgetTreeDragDropPayload::StaticTypeDesc()
{
    static const ImWidgetV4::Reflection::FPropertyDesc properties[] = {
        ImWidgetV4::Reflection::MakeMemberProperty<WidgetTreeDragDropPayload, std::string, &WidgetTreeDragDropPayload::WidgetId>(
            "WidgetTreeDragDropPayload",
            "WidgetId",
            ImWidgetV4::Reflection::EPropertyKind::String,
            "std::string",
            "Widget id"),
        ImWidgetV4::Reflection::MakeMemberProperty<WidgetTreeDragDropPayload, std::string, &WidgetTreeDragDropPayload::Label>(
            "WidgetTreeDragDropPayload",
            "Label",
            ImWidgetV4::Reflection::EPropertyKind::String,
            "std::string",
            "Label")
    };

    static const ImWidgetV4::Reflection::FTypeDesc typeDesc {
        "WidgetTreeDragDropPayload",
        &ImWidgetV4::FDragDropPayload::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const ImWidgetV4::Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

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
