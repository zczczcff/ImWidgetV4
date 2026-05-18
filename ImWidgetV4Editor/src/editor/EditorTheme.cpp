#include "EditorTheme.h"

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

FColor GetEditorSurfaceBackgroundColor()
{
    return FColor::FromBytes(18, 23, 29);
}

FColor GetEditorSurfaceAltBackgroundColor()
{
    return FColor::FromBytes(27, 33, 41);
}

FColor GetEditorSurfaceTabStripColor()
{
    return FColor::FromBytes(27, 33, 41);
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
    return FColor::FromBytes(180, 190, 204);
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

FImageStyle MakeEditorPlainIconStyle(const FImageStyle& baseStyle)
{
    FImageStyle style = baseStyle;
    style.BackgroundColor = FColor::Transparent;
    style.BorderColor = FColor::Transparent;
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    return style;
}

FScrollBoxStyle MakeEditorHostScrollStyle(const FScrollBoxStyle& baseStyle, const FMargin& padding)
{
    FScrollBoxStyle style = baseStyle;
    style.BackgroundColor = GetEditorSurfaceBackgroundColor();
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
    style.TextColor = FColor::FromBytes(196, 205, 217);
    style.SelectionBackgroundColor = GetEditorSelectionFillColor();
    style.Padding = padding;
    style.MinDesiredSize = minDesiredSize;
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.LineSpacing = 1.1f;
    return style;
}

FTextOutlineViewStyle MakeEditorDockOutlineStyle(const FTextOutlineViewStyle& baseStyle)
{
    FTextOutlineViewStyle style = baseStyle;
    style.Padding = FMargin(6.0f);
    style.RowPadding = FMargin(5.0f, 6.0f, 3.0f, 3.0f);
    style.MinDesiredSize = FVector2(220.0f, 180.0f);
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.RowHeight = 22.0f;
    style.IndentWidth = 16.0f;
    style.IndicatorSize = 9.0f;
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
    style.ActiveTabColor = FColor::FromBytes(63, 90, 128);
    return style;
}

} // namespace ImWidgetV4Editor
