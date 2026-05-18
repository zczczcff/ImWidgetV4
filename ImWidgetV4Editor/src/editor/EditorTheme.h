#pragma once

#include <imwidgetv4/widgets/ButtonStyle.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>

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

ImWidgetV4::FButtonStyle MakeEditorTitleBarButtonStyle(bool bHighlighted = false);
ImWidgetV4::FButtonStyle MakeEditorTitleBarIconButtonStyle();
ImWidgetV4::FImageStyle MakeEditorPlainIconStyle(const ImWidgetV4::FImageStyle& baseStyle = ImWidgetV4::FImageStyle());
ImWidgetV4::FScrollBoxStyle MakeEditorHostScrollStyle(
    const ImWidgetV4::FScrollBoxStyle& baseStyle = ImWidgetV4::FScrollBoxStyle(),
    const ImWidgetV4::FMargin& padding = ImWidgetV4::FMargin(0.0f));
ImWidgetV4::FTextListStyle MakeEditorCodeTextListStyle(
    const ImWidgetV4::FTextListStyle& baseStyle = ImWidgetV4::FTextListStyle(),
    const ImWidgetV4::FMargin& padding = ImWidgetV4::FMargin(12.0f),
    const ImWidgetV4::FVector2& minDesiredSize = ImWidgetV4::FVector2(0.0f, 180.0f));
ImWidgetV4::FTextOutlineViewStyle MakeEditorDockOutlineStyle(
    const ImWidgetV4::FTextOutlineViewStyle& baseStyle = ImWidgetV4::FTextOutlineViewStyle());
ImWidgetV4::FTabViewStyle MakeEditorWorkspaceTabStyle(
    const ImWidgetV4::FTabViewStyle& baseStyle = ImWidgetV4::FTabViewStyle());

} // namespace ImWidgetV4Editor
