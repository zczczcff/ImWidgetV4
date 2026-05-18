#pragma once

#include <imwidgetv4/style/StyleSet.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ColorPicker.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/ListView.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/TitleBar.h>

namespace ImWidgetV4 {

FButtonStyle ResolveButtonStyle(const FStyleSet& styleSet);
FCheckBoxStyle ResolveCheckBoxStyle(const FStyleSet& styleSet);
FColorPickerStyle ResolveColorPickerStyle(const FStyleSet& styleSet);
FComboBoxStyle ResolveComboBoxStyle(const FStyleSet& styleSet);
FDesignerSurfaceStyle ResolveDesignerSurfaceStyle(const FStyleSet& styleSet);
FEditableTextStyle ResolveEditableTextStyle(const FStyleSet& styleSet);
FExpandableBoxStyle ResolveExpandableBoxStyle(const FStyleSet& styleSet);
FImageStyle ResolveImageStyle(const FStyleSet& styleSet);
FListViewStyle ResolveListViewStyle(const FStyleSet& styleSet);
FOutlineViewStyle ResolveOutlineViewStyle(const FStyleSet& styleSet);
FPopupMenuStyle ResolvePopupMenuStyle(const FStyleSet& styleSet);
FScrollBoxStyle ResolveScrollBoxStyle(const FStyleSet& styleSet);
FSliderStyle ResolveSliderStyle(const FStyleSet& styleSet);
FHorizontalSplitterStyle ResolveHorizontalSplitterStyle(const FStyleSet& styleSet);
FVerticalSplitterStyle ResolveVerticalSplitterStyle(const FStyleSet& styleSet);
FSwitchStyle ResolveSwitchStyle(const FStyleSet& styleSet);
FTabViewStyle ResolveTabViewStyle(const FStyleSet& styleSet);
FTextBlockStyle ResolveTextBlockStyle(const FStyleSet& styleSet);
FTextListStyle ResolveTextListStyle(const FStyleSet& styleSet);
FTextOutlineViewStyle ResolveTextOutlineViewStyle(const FStyleSet& styleSet);
FTitleBarStyle ResolveTitleBarStyle(const FStyleSet& styleSet);

} // namespace ImWidgetV4
