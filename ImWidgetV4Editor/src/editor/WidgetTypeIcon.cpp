#include "WidgetTypeIcon.h"

#include <unordered_map>

namespace ImWidgetV4Editor {

std::optional<ImWidgetV4::ECoreIcon> TryGetWidgetTypeIcon(const std::string& typeName)
{
    using ImWidgetV4::ECoreIcon;

    static const std::unordered_map<std::string, ECoreIcon> GTypeToIcon = {
        {"ImCanvasPanel", ECoreIcon::CanvasPanel},
        {"ImHorizontalBox", ECoreIcon::HorizontalBox},
        {"ImVerticalBox", ECoreIcon::VerticalBox},
        {"ImScrollBox", ECoreIcon::ScrollBox},
        {"ImTabView", ECoreIcon::TabView},
        {"ImExpandableBox", ECoreIcon::ExpandableBox},
        {"ImTextBlock", ECoreIcon::TextBlock},
        {"ImButton", ECoreIcon::Button},
        {"ImCheckBox", ECoreIcon::CheckBox},
        {"ImSwitch", ECoreIcon::Switch},
        {"ImSlider", ECoreIcon::Slider},
        {"ImEditableText", ECoreIcon::EditableText},
        {"ImImage", ECoreIcon::Image},
        {"ImTextList", ECoreIcon::TextList},
        {"ImTextOutlineView", ECoreIcon::TextOutlineView},
        {"ImOutlineView", ECoreIcon::OutlineView},
        {"ImUserWidget", ECoreIcon::UserWidget},
        {"ImDesignerSurface", ECoreIcon::DesignerSurface},
        {"ImHorizontalSplitter", ECoreIcon::HorizontalSplitter},
        {"ImVerticalSplitter", ECoreIcon::VerticalSplitter}
    };

    const auto it = GTypeToIcon.find(typeName);
    if (it == GTypeToIcon.end()) {
        return std::nullopt;
    }

    return it->second;
}

} // namespace ImWidgetV4Editor
