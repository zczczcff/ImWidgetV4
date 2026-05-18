#include "EditorTheme.h"

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

FColor GetEditorSurfaceBackgroundColor()
{
    return FColor::FromBytes(18, 23, 29);
}

FColor GetEditorSurfaceAltBackgroundColor()
{
    return FColor::FromBytes(30, 36, 44);
}

FColor GetEditorSurfaceTabStripColor()
{
    return FColor::FromBytes(24, 29, 36);
}

FColor GetEditorAccentColor()
{
    return FColor::FromBytes(103, 177, 255);
}

FColor GetEditorSelectionFillColor()
{
    return FColor::FromBytes(72, 104, 146, 148);
}

FColor GetEditorPanelTitleColor()
{
    return FColor::FromBytes(238, 242, 247);
}

FColor GetEditorPanelBodyColor()
{
    return FColor::FromBytes(164, 174, 188);
}

FColor GetEditorTitleBarTextColor()
{
    return FColor::FromBytes(232, 238, 246);
}

FColor GetEditorTitleBarMutedTextColor()
{
    return FColor::FromBytes(150, 160, 172);
}

FColor GetEditorTitleBarDisabledTextColor()
{
    return FColor::FromBytes(132, 140, 150);
}

FColor GetEditorDangerColor()
{
    return FColor::FromBytes(239, 103, 103);
}

FColor GetEditorSuccessColor()
{
    return FColor::FromBytes(125, 204, 138);
}

FColor GetEditorWarningColor()
{
    return FColor::FromBytes(230, 184, 104);
}

FColor GetEditorInspectorLabelColor()
{
    return FColor::FromBytes(224, 230, 237);
}

FColor GetEditorInspectorCompactLabelColor()
{
    return FColor::FromBytes(158, 168, 180);
}

FColor GetEditorTitleBarIconColor()
{
    return FColor::FromBytes(235, 242, 250);
}

FColor GetEditorTitleBarIconDisabledColor()
{
    return GetEditorTitleBarDisabledTextColor();
}

FColor GetEditorTreeIconColor()
{
    return FColor::FromBytes(214, 222, 234);
}

FButtonStyle MakeEditorTitleBarButtonStyle(bool bHighlighted)
{
    FButtonStyle style = FButtonStyle::CreatePrimary();
    const FColor textColor = GetEditorTitleBarTextColor();
    const FColor transparent = FColor::Transparent;
    const FColor baseColor = bHighlighted ? FColor::FromBytes(72, 104, 146, 116) : transparent;
    style.Normal = FButtonStateStyle(baseColor, transparent, textColor, 0.0f, 0.0f, false);
    style.Hovered = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 24), transparent, textColor, 0.0f, 0.0f, false);
    style.Pressed = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 38), transparent, textColor, 0.0f, 0.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(transparent, transparent, GetEditorTitleBarDisabledTextColor(), 0.0f, 0.0f, false);
    return style;
}

FButtonStyle MakeEditorDialogButtonStyle(bool bPrimary, const FButtonStyle& baseStyle)
{
    FButtonStyle style = baseStyle;
    if (bPrimary) {
        style = FButtonStyle::CreatePrimary();
        return style;
    }

    style.Normal = FButtonStateStyle(FColor::FromBytes(31, 37, 46), FColor::FromBytes(16, 19, 23), GetEditorPanelTitleColor(), 5.0f, 1.0f, false);
    style.Hovered = FButtonStateStyle(FColor::FromBytes(39, 46, 56), FColor::FromBytes(16, 19, 23), GetEditorPanelTitleColor(), 5.0f, 1.0f, false);
    style.Pressed = FButtonStateStyle(FColor::FromBytes(24, 31, 40), FColor::FromBytes(16, 19, 23), GetEditorPanelTitleColor(), 5.0f, 1.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(FColor::FromBytes(46, 52, 61), FColor::FromBytes(16, 19, 23), FColor::FromBytes(188, 196, 207), 5.0f, 1.0f, false);
    return style;
}

FButtonStyle MakeEditorTitleBarIconButtonStyle()
{
    FButtonStyle style = FButtonStyle::CreatePrimary();
    const FColor transparent = FColor::Transparent;
    const FColor textColor = GetEditorTitleBarTextColor();
    style.Normal = FButtonStateStyle(transparent, transparent, textColor, 0.0f, 0.0f, false);
    style.Hovered = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 18), transparent, textColor, 0.0f, 0.0f, false);
    style.Pressed = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 30), transparent, textColor, 0.0f, 0.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(transparent, transparent, GetEditorTitleBarDisabledTextColor(), 0.0f, 0.0f, false);
    return style;
}

FButtonStyle MakeEditorPaletteItemButtonStyle(const FButtonStyle& baseStyle)
{
    FButtonStyle style = baseStyle;
    const FColor background = FColor::FromBytes(34, 40, 49);
    const FColor hovered = FColor::FromBytes(43, 50, 61);
    const FColor pressed = FColor::FromBytes(52, 61, 74);
    const FColor selectedBorder = FColor::FromBytes(72, 104, 146);
    const FColor border = FColor::FromBytes(20, 24, 30);
    const FColor textColor = GetEditorPanelTitleColor();
    style.Normal = FButtonStateStyle(background, border, textColor, 4.0f, 1.0f, false);
    style.Hovered = FButtonStateStyle(hovered, selectedBorder, textColor, 4.0f, 1.0f, false);
    style.Pressed = FButtonStateStyle(pressed, GetEditorAccentColor(), textColor, 4.0f, 1.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(
        FColor::FromBytes(42, 47, 55),
        border,
        FColor::FromBytes(128, 136, 146),
        4.0f,
        1.0f,
        false);
    return style;
}

FImageStyle MakeEditorPlainIconStyle(const FImageStyle& baseStyle)
{
    FImageStyle style = baseStyle;
    style.BackgroundColor = FColor::Transparent;
    style.BorderColor = FColor::Transparent;
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    return style;
}

FPopupMenuStyle MakeEditorPopupMenuStyle(const FPopupMenuStyle& baseStyle)
{
    FPopupMenuStyle style = baseStyle;
    style.CornerRadius = 6.0f;
    return style;
}

FWindowStyle MakeEditorPopupWindowStyle(const FWindowStyle& baseStyle)
{
    FWindowStyle style = baseStyle;
    style.CornerRadius = 6.0f;
    style.BorderThickness = 1.0f;
    style.bDrawShadow = false;
    style.BackgroundColor = GetEditorSurfaceAltBackgroundColor();
    style.BorderColor = FColor::FromBytes(16, 19, 23);
    return style;
}

FEditableTextStyle MakeEditorInspectorEditableTextStyle(const FEditableTextStyle& baseStyle)
{
    FEditableTextStyle style = baseStyle;
    style.BackgroundColor = FColor::FromBytes(31, 37, 46);
    style.HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
    style.FocusedBackgroundColor = FColor::FromBytes(24, 31, 40);
    style.DisabledBackgroundColor = FColor::FromBytes(46, 52, 61);
    style.BorderColor = FColor::FromBytes(16, 19, 23);
    style.FocusedOutlineColor = GetEditorAccentColor();
    style.TextColor = FColor::FromBytes(238, 241, 245);
    style.DisabledTextColor = FColor::FromBytes(188, 196, 207);
    style.HintTextColor = FColor::FromBytes(135, 145, 157);
    style.Padding = FMargin(10.0f);
    style.MinDesiredSize = FVector2(0.0f, 34.0f);
    style.CornerRadius = 5.0f;
    style.BorderThickness = 1.0f;
    style.FontSize = 14.0f;
    return style;
}

FComboBoxStyle MakeEditorInspectorComboBoxStyle(const FComboBoxStyle& baseStyle)
{
    FComboBoxStyle style = baseStyle;
    style.BackgroundColor = FColor::FromBytes(31, 37, 46);
    style.HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
    style.PressedBackgroundColor = FColor::FromBytes(24, 31, 40);
    style.DisabledBackgroundColor = FColor::FromBytes(46, 52, 61);
    style.BorderColor = FColor::FromBytes(16, 19, 23);
    style.FocusedOutlineColor = GetEditorAccentColor();
    style.TextColor = FColor::FromBytes(238, 241, 245);
    style.PlaceholderTextColor = FColor::FromBytes(135, 145, 157);
    style.DisabledTextColor = FColor::FromBytes(188, 196, 207);
    style.ArrowColor = FColor::FromBytes(220, 227, 235);
    style.PopupRowHoveredColor = FColor::FromBytes(46, 58, 76);
    style.PopupRowSelectedColor = FColor::FromBytes(78, 126, 196);
    style.PopupRowSelectedHoveredColor = FColor::FromBytes(96, 149, 221);
    style.PopupOutlineColor = FColor::FromBytes(16, 19, 23);
    style.Padding = FMargin(10.0f);
    style.FontSize = 14.0f;
    style.BorderThickness = 1.0f;
    style.CornerRadius = 5.0f;
    style.ArrowSize = 9.0f;
    style.PopupItemHeight = 28.0f;
    style.PopupMaxVisibleItems = 8.0f;
    style.MinDesiredSize = FVector2(0.0f, 34.0f);
    return style;
}

FSwitchStyle MakeEditorInspectorSwitchStyle(const FSwitchStyle& baseStyle)
{
    FSwitchStyle style = baseStyle;
    style.DesiredSize = FVector2(46.0f, 24.0f);
    style.BorderThickness = 1.0f;
    style.ThumbInset = 3.0f;
    return style;
}

FScrollBoxStyle MakeEditorHostScrollStyle(const FScrollBoxStyle& baseStyle, const FMargin& padding)
{
    FScrollBoxStyle style = baseStyle;
    style.BackgroundColor = GetEditorSurfaceAltBackgroundColor();
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.Padding = padding;
    return style;
}

FTextListStyle MakeEditorCodeTextListStyle(
    const FTextListStyle& baseStyle,
    const FMargin& padding,
    const FVector2& minDesiredSize)
{
    FTextListStyle style = baseStyle;
    style.BackgroundColor = GetEditorSurfaceBackgroundColor();
    style.BorderColor = FColor::Transparent;
    style.FocusedOutlineColor = GetEditorAccentColor();
    style.TextColor = FColor::FromBytes(188, 198, 210);
    style.SelectionBackgroundColor = GetEditorSelectionFillColor();
    style.Padding = padding;
    style.MinDesiredSize = minDesiredSize;
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.LineSpacing = 1.1f;
    return style;
}

FTextListStyle MakeEditorInspectorProbeTextListStyle(const FTextListStyle& baseStyle)
{
    FTextListStyle style = baseStyle;
    style.BackgroundColor = GetEditorSurfaceBackgroundColor();
    style.BorderColor = FColor::FromBytes(16, 19, 23);
    style.FocusedOutlineColor = GetEditorAccentColor();
    style.TextColor = FColor::FromBytes(196, 205, 217);
    style.SelectionBackgroundColor = GetEditorSelectionFillColor();
    style.Padding = FMargin(10.0f);
    style.MinDesiredSize = FVector2(0.0f, 200.0f);
    style.CornerRadius = 5.0f;
    style.BorderThickness = 1.0f;
    style.FontSize = 13.0f;
    style.LineSpacing = 1.1f;
    return style;
}

FTextOutlineViewStyle MakeEditorDockOutlineStyle(const FTextOutlineViewStyle& baseStyle)
{
    FTextOutlineViewStyle style = baseStyle;
    style.Padding = FMargin(4.0f);
    style.RowPadding = FMargin(6.0f, 6.0f, 4.0f, 4.0f);
    style.BackgroundColor = GetEditorSurfaceAltBackgroundColor();
    style.BorderColor = FColor::Transparent;
    style.HoveredRowColor = FColor::FromBytes(43, 50, 61);
    style.SelectedRowColor = FColor::FromBytes(60, 86, 122);
    style.SelectedFocusedRowColor = FColor::FromBytes(74, 106, 150);
    style.TextColor = GetEditorPanelTitleColor();
    style.MinDesiredSize = FVector2(220.0f, 180.0f);
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.RowHeight = 22.0f;
    style.IndentWidth = 16.0f;
    style.IndicatorSize = 9.0f;
    return style;
}

FOutlineViewStyle MakeEditorInspectorOutlineStyle(const FOutlineViewStyle& baseStyle)
{
    FOutlineViewStyle style = baseStyle;
    style.Padding = FMargin(0.0f);
    style.RowPadding = FMargin(6.0f, 8.0f, 4.0f, 4.0f);
    style.BackgroundColor = GetEditorSurfaceAltBackgroundColor();
    style.BorderColor = FColor::Transparent;
    style.FocusedOutlineColor = GetEditorAccentColor();
    style.HoveredRowColor = FColor::FromBytes(43, 50, 61);
    style.SelectedRowColor = FColor::FromBytes(60, 86, 122);
    style.SelectedFocusedRowColor = FColor::FromBytes(74, 106, 150);
    style.BorderThickness = 1.0f;
    style.CornerRadius = 0.0f;
    style.IndentWidth = 16.0f;
    style.IndicatorSize = 10.0f;
    style.IndicatorSpacing = 6.0f;
    style.RowMinHeight = 30.0f;
    style.MinDesiredSize = FVector2(280.0f, 220.0f);
    return style;
}

FTabViewStyle MakeEditorWorkspaceTabStyle(const FTabViewStyle& baseStyle)
{
    FTabViewStyle style = baseStyle;
    style.Padding = FMargin(0.0f);
    style.TabHeight = 28.0f;
    style.TabMinWidth = 110.0f;
    style.TabSpacing = 0.0f;
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.TabStripBackgroundColor = GetEditorSurfaceTabStripColor();
    style.BackgroundColor = GetEditorSurfaceBackgroundColor();
    style.TabColor = FColor::FromBytes(33, 39, 47);
    style.TabHoveredColor = FColor::FromBytes(43, 50, 60);
    style.TabPressedColor = FColor::FromBytes(28, 34, 41);
    style.ActiveTabColor = FColor::FromBytes(67, 96, 138);
    style.TextColor = FColor::FromBytes(180, 189, 201);
    style.ActiveTextColor = FColor::FromBytes(241, 246, 252);
    style.DisabledTextColor = FColor::FromBytes(118, 126, 137);
    return style;
}

FTabViewStyle MakeEditorDockTabStyle(const FTabViewStyle& baseStyle)
{
    FTabViewStyle style = baseStyle;
    style.Padding = FMargin(0.0f);
    style.TabPadding = FMargin(4.0f, 4.0f, 2.0f, 2.0f);
    style.TabHeight = 20.0f;
    style.TabMinWidth = 64.0f;
    style.TabSpacing = 0.0f;
    style.FontSize = 16.0f;
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.BackgroundColor = GetEditorSurfaceBackgroundColor();
    style.TabStripBackgroundColor = GetEditorSurfaceTabStripColor();
    style.TabColor = FColor::FromBytes(34, 40, 49);
    style.TabHoveredColor = FColor::FromBytes(44, 51, 62);
    style.TabPressedColor = FColor::FromBytes(30, 35, 43);
    style.ActiveTabColor = FColor::FromBytes(67, 96, 138);
    style.TextColor = FColor::FromBytes(176, 186, 198);
    style.ActiveTextColor = FColor::FromBytes(241, 246, 252);
    return style;
}

FTitleBarStyle MakeEditorTitleBarStyle(const FTitleBarStyle& baseStyle)
{
    FTitleBarStyle style = baseStyle;
    style.Height = 24.0f;
    style.Padding = FMargin(4.0f, 0.0f, 0.0f, 0.0f);
    style.ItemSpacing = 4.0f;
    style.SystemButtonSize = 34.0f;
    style.MinDesiredSize = FVector2(0.0f, 24.0f);
    return style;
}

FHorizontalSplitterStyle MakeEditorHorizontalSplitterStyle(const FHorizontalSplitterStyle& baseStyle)
{
    FHorizontalSplitterStyle style = baseStyle;
    style.BarWidth = 5.0f;
    style.Color = FColor::FromBytes(44, 51, 61);
    style.HoveredColor = FColor::FromBytes(70, 82, 99);
    style.ActiveColor = GetEditorAccentColor();
    return style;
}

FVerticalSplitterStyle MakeEditorVerticalSplitterStyle(const FVerticalSplitterStyle& baseStyle)
{
    FVerticalSplitterStyle style = baseStyle;
    style.BarHeight = 5.0f;
    style.Color = FColor::FromBytes(44, 51, 61);
    style.HoveredColor = FColor::FromBytes(70, 82, 99);
    style.ActiveColor = GetEditorAccentColor();
    return style;
}

} // namespace ImWidgetV4Editor
