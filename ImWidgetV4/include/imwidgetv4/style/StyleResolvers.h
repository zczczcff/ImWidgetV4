#pragma once

#include <imwidgetv4/style/StyleSet.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TitleBar.h>

namespace ImWidgetV4 {

FButtonStyle ResolveButtonStyle(const FStyleSet& styleSet);
FCheckBoxStyle ResolveCheckBoxStyle(const FStyleSet& styleSet);
FComboBoxStyle ResolveComboBoxStyle(const FStyleSet& styleSet);
FEditableTextStyle ResolveEditableTextStyle(const FStyleSet& styleSet);
FPopupMenuStyle ResolvePopupMenuStyle(const FStyleSet& styleSet);
FScrollBoxStyle ResolveScrollBoxStyle(const FStyleSet& styleSet);
FSwitchStyle ResolveSwitchStyle(const FStyleSet& styleSet);
FTabViewStyle ResolveTabViewStyle(const FStyleSet& styleSet);
FTitleBarStyle ResolveTitleBarStyle(const FStyleSet& styleSet);

} // namespace ImWidgetV4
