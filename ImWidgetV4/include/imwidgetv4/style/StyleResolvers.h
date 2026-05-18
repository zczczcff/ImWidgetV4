#pragma once

#include <imwidgetv4/style/StyleSet.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/TitleBar.h>

namespace ImWidgetV4 {

FButtonStyle ResolveButtonStyle(const FStyleSet& styleSet);
FEditableTextStyle ResolveEditableTextStyle(const FStyleSet& styleSet);
FTitleBarStyle ResolveTitleBarStyle(const FStyleSet& styleSet);

} // namespace ImWidgetV4
