#pragma once

#include <imwidgetv4/core/Window.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>

namespace ImWidgetV4Editor {

ImWidgetV4::FColor GetEditorSurfaceBackgroundColor();
ImWidgetV4::FColor GetEditorSurfaceAltBackgroundColor();
ImWidgetV4::FColor GetEditorSurfaceTabStripColor();
ImWidgetV4::FColor GetEditorAccentColor();
ImWidgetV4::FColor GetEditorSelectionFillColor();
ImWidgetV4::FColor GetEditorPanelTitleColor();
ImWidgetV4::FColor GetEditorPanelBodyColor();
ImWidgetV4::FColor GetEditorTitleBarTextColor();
ImWidgetV4::FColor GetEditorTitleBarMutedTextColor();
ImWidgetV4::FColor GetEditorTitleBarDisabledTextColor();
ImWidgetV4::FColor GetEditorDangerColor();
ImWidgetV4::FColor GetEditorSuccessColor();
ImWidgetV4::FColor GetEditorWarningColor();
ImWidgetV4::FColor GetEditorInspectorLabelColor();
ImWidgetV4::FColor GetEditorInspectorCompactLabelColor();
ImWidgetV4::FColor GetEditorTitleBarIconColor();
ImWidgetV4::FColor GetEditorTitleBarIconDisabledColor();
ImWidgetV4::FColor GetEditorTreeIconColor();

ImWidgetV4::FButtonStyle MakeEditorTitleBarButtonStyle(bool bHighlighted = false);
ImWidgetV4::FButtonStyle MakeEditorTitleBarIconButtonStyle();
ImWidgetV4::FButtonStyle MakeEditorDialogButtonStyle(
    bool bPrimary,
    const ImWidgetV4::FButtonStyle& baseStyle = ImWidgetV4::FButtonStyle());
ImWidgetV4::FImageStyle MakeEditorPlainIconStyle(const ImWidgetV4::FImageStyle& baseStyle = ImWidgetV4::FImageStyle());
ImWidgetV4::FPopupMenuStyle MakeEditorPopupMenuStyle(
    const ImWidgetV4::FPopupMenuStyle& baseStyle = ImWidgetV4::FPopupMenuStyle());
ImWidgetV4::FWindowStyle MakeEditorPopupWindowStyle(
    const ImWidgetV4::FWindowStyle& baseStyle = ImWidgetV4::FWindowStyle());
ImWidgetV4::FEditableTextStyle MakeEditorInspectorEditableTextStyle(
    const ImWidgetV4::FEditableTextStyle& baseStyle = ImWidgetV4::FEditableTextStyle());
ImWidgetV4::FComboBoxStyle MakeEditorInspectorComboBoxStyle(
    const ImWidgetV4::FComboBoxStyle& baseStyle = ImWidgetV4::FComboBoxStyle());
ImWidgetV4::FSwitchStyle MakeEditorInspectorSwitchStyle(
    const ImWidgetV4::FSwitchStyle& baseStyle = ImWidgetV4::FSwitchStyle());
ImWidgetV4::FScrollBoxStyle MakeEditorHostScrollStyle(
    const ImWidgetV4::FScrollBoxStyle& baseStyle = ImWidgetV4::FScrollBoxStyle(),
    const ImWidgetV4::FMargin& padding = ImWidgetV4::FMargin(0.0f));
ImWidgetV4::FTextListStyle MakeEditorCodeTextListStyle(
    const ImWidgetV4::FTextListStyle& baseStyle = ImWidgetV4::FTextListStyle(),
    const ImWidgetV4::FMargin& padding = ImWidgetV4::FMargin(12.0f),
    const ImWidgetV4::FVector2& minDesiredSize = ImWidgetV4::FVector2(0.0f, 180.0f));
ImWidgetV4::FTextListStyle MakeEditorInspectorProbeTextListStyle(
    const ImWidgetV4::FTextListStyle& baseStyle = ImWidgetV4::FTextListStyle());
ImWidgetV4::FTextOutlineViewStyle MakeEditorDockOutlineStyle(
    const ImWidgetV4::FTextOutlineViewStyle& baseStyle = ImWidgetV4::FTextOutlineViewStyle());
ImWidgetV4::FOutlineViewStyle MakeEditorInspectorOutlineStyle(
    const ImWidgetV4::FOutlineViewStyle& baseStyle = ImWidgetV4::FOutlineViewStyle());
ImWidgetV4::FTabViewStyle MakeEditorWorkspaceTabStyle(
    const ImWidgetV4::FTabViewStyle& baseStyle = ImWidgetV4::FTabViewStyle());
ImWidgetV4::FTabViewStyle MakeEditorDockTabStyle(
    const ImWidgetV4::FTabViewStyle& baseStyle = ImWidgetV4::FTabViewStyle());
ImWidgetV4::FTitleBarStyle MakeEditorTitleBarStyle(
    const ImWidgetV4::FTitleBarStyle& baseStyle = ImWidgetV4::FTitleBarStyle());
ImWidgetV4::FHorizontalSplitterStyle MakeEditorHorizontalSplitterStyle(
    const ImWidgetV4::FHorizontalSplitterStyle& baseStyle = ImWidgetV4::FHorizontalSplitterStyle());
ImWidgetV4::FVerticalSplitterStyle MakeEditorVerticalSplitterStyle(
    const ImWidgetV4::FVerticalSplitterStyle& baseStyle = ImWidgetV4::FVerticalSplitterStyle());

} // namespace ImWidgetV4Editor
