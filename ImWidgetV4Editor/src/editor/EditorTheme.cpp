#include "EditorTheme.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/style/StyleSet.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

namespace {

std::string GEditorActiveThemeName = "Default";
FStyleSet GEditorActiveStyleSet;
bool bHasEditorActiveStyleSet = false;

bool IsLightEditorTheme()
{
    return GEditorActiveThemeName == "Light" ||
        GEditorActiveThemeName == "Editor Light Gray";
}

FColor GetEditorColorToken(const char* token, const FColor& fallback)
{
    return bHasEditorActiveStyleSet
        ? GEditorActiveStyleSet.GetColor(token, fallback)
        : fallback;
}

constexpr const char* GEditorBlueGreenThemeJson = R"json(
{
  "Name": "Editor Blue Green",
  "BaseTheme": "Dark",
  "StyleSet": {
    "Colors": {
      "Color.Editor.Surface.Background": {"Bytes": [16, 27, 29]},
      "Color.Editor.Surface.AltBackground": {"Bytes": [24, 42, 44]},
      "Color.Editor.Surface.TabStrip": {"Bytes": [20, 36, 38]},
      "Color.Editor.Accent": {"Bytes": [64, 196, 181]},
      "Color.Editor.SelectionFill": {"Bytes": [72, 104, 146, 148]},
      "Color.Editor.Panel.Title": {"Bytes": [238, 242, 247]},
      "Color.Editor.Panel.Body": {"Bytes": [164, 174, 188]},
      "Color.Editor.TitleBar.Text": {"Bytes": [232, 238, 246]},
      "Color.Editor.TitleBar.MutedText": {"Bytes": [150, 160, 172]},
      "Color.Editor.TitleBar.DisabledText": {"Bytes": [132, 140, 150]},
      "Color.Editor.Danger": {"Bytes": [239, 103, 103]},
      "Color.Editor.Success": {"Bytes": [125, 204, 138]},
      "Color.Editor.Warning": {"Bytes": [230, 184, 104]},
      "Color.Editor.Inspector.Label": {"Bytes": [224, 230, 237]},
      "Color.Editor.Inspector.CompactLabel": {"Bytes": [158, 168, 180]},
      "Color.Editor.TitleBar.Icon": {"Bytes": [235, 242, 250]},
      "Color.Editor.Tree.Icon": {"Bytes": [214, 222, 234]}
    }
  }
}
)json";

constexpr const char* GEditorLightGrayThemeJson = R"json(
{
  "Name": "Editor Light Gray",
  "BaseTheme": "Light",
  "StyleSet": {
    "Colors": {
      "Color.Editor.Surface.Background": {"Bytes": [248, 249, 251]},
      "Color.Editor.Surface.AltBackground": {"Bytes": [238, 240, 244]},
      "Color.Editor.Surface.TabStrip": {"Bytes": [232, 235, 240]},
      "Color.Editor.Accent": {"Bytes": [0, 120, 215]},
      "Color.Editor.SelectionFill": {"Bytes": [162, 205, 255, 200]},
      "Color.Editor.Panel.Title": {"Bytes": [36, 36, 36]},
      "Color.Editor.Panel.Body": {"Bytes": [108, 116, 126]},
      "Color.Editor.TitleBar.Text": {"Bytes": [48, 52, 58]},
      "Color.Editor.TitleBar.MutedText": {"Bytes": [118, 126, 136]},
      "Color.Editor.TitleBar.DisabledText": {"Bytes": [154, 160, 170]},
      "Color.Editor.Danger": {"Bytes": [212, 58, 76]},
      "Color.Editor.Success": {"Bytes": [40, 168, 96]},
      "Color.Editor.Warning": {"Bytes": [212, 132, 24]},
      "Color.Editor.Inspector.Label": {"Bytes": [58, 64, 72]},
      "Color.Editor.Inspector.CompactLabel": {"Bytes": [120, 126, 136]},
      "Color.Editor.TitleBar.Icon": {"Bytes": [60, 66, 74]},
      "Color.Editor.Tree.Icon": {"Bytes": [96, 104, 114]}
    }
  }
}
)json";

void RegisterEditorThemePackFromJson(ImApplication& application, const char* themeJson)
{
    std::string error;
    FThemePack themePack = FStyleSetFactory::CreateThemePackFromJsonString(themeJson, &error);
    if (!themePack.Name.empty()) {
        application.RegisterThemePack(std::move(themePack));
    }
}

} // namespace

void RegisterEditorThemePacks(ImApplication& application)
{
    RegisterEditorThemePackFromJson(application, GEditorBlueGreenThemeJson);
    RegisterEditorThemePackFromJson(application, GEditorLightGrayThemeJson);
}

void SetEditorActiveThemeName(const std::string& themeName)
{
    GEditorActiveThemeName = themeName.empty() ? "Default" : themeName;
    GEditorActiveStyleSet.Clear();
    bHasEditorActiveStyleSet = false;
}

void SetEditorActiveTheme(const std::string& themeName, const FStyleSet& styleSet)
{
    GEditorActiveThemeName = themeName.empty() ? "Default" : themeName;
    GEditorActiveStyleSet.Clear();
    GEditorActiveStyleSet.Merge(styleSet);
    bHasEditorActiveStyleSet = true;
}

const std::string& GetEditorActiveThemeName()
{
    return GEditorActiveThemeName;
}

FColor GetEditorSurfaceBackgroundColor()
{
    return GetEditorColorToken(
        "Color.Editor.Surface.Background",
        IsLightEditorTheme() ? FColor::FromBytes(248, 249, 251) : FColor::FromBytes(18, 23, 29));
}

FColor GetEditorSurfaceAltBackgroundColor()
{
    return GetEditorColorToken(
        "Color.Editor.Surface.AltBackground",
        IsLightEditorTheme() ? FColor::FromBytes(238, 240, 244) : FColor::FromBytes(30, 36, 44));
}

FColor GetEditorSurfaceTabStripColor()
{
    return GetEditorColorToken(
        "Color.Editor.Surface.TabStrip",
        IsLightEditorTheme() ? FColor::FromBytes(232, 235, 240) : FColor::FromBytes(24, 29, 36));
}

FColor GetEditorAccentColor()
{
    return GetEditorColorToken(
        "Color.Editor.Accent",
        IsLightEditorTheme() ? FColor::FromBytes(0, 120, 215) : FColor::FromBytes(103, 177, 255));
}

FColor GetEditorSelectionFillColor()
{
    return GetEditorColorToken(
        "Color.Editor.SelectionFill",
        IsLightEditorTheme() ? FColor::FromBytes(162, 205, 255, 200) : FColor::FromBytes(72, 104, 146, 148));
}

FColor GetEditorPanelTitleColor()
{
    return GetEditorColorToken(
        "Color.Editor.Panel.Title",
        IsLightEditorTheme() ? FColor::FromBytes(36, 36, 36) : FColor::FromBytes(238, 242, 247));
}

FColor GetEditorPanelBodyColor()
{
    return GetEditorColorToken(
        "Color.Editor.Panel.Body",
        IsLightEditorTheme() ? FColor::FromBytes(108, 116, 126) : FColor::FromBytes(164, 174, 188));
}

FColor GetEditorTitleBarTextColor()
{
    return GetEditorColorToken(
        "Color.Editor.TitleBar.Text",
        IsLightEditorTheme() ? FColor::FromBytes(48, 52, 58) : FColor::FromBytes(232, 238, 246));
}

FColor GetEditorTitleBarMutedTextColor()
{
    return GetEditorColorToken(
        "Color.Editor.TitleBar.MutedText",
        IsLightEditorTheme() ? FColor::FromBytes(118, 126, 136) : FColor::FromBytes(150, 160, 172));
}

FColor GetEditorTitleBarDisabledTextColor()
{
    return GetEditorColorToken(
        "Color.Editor.TitleBar.DisabledText",
        IsLightEditorTheme() ? FColor::FromBytes(154, 160, 170) : FColor::FromBytes(132, 140, 150));
}

FColor GetEditorDangerColor()
{
    return GetEditorColorToken(
        "Color.Editor.Danger",
        IsLightEditorTheme() ? FColor::FromBytes(212, 58, 76) : FColor::FromBytes(239, 103, 103));
}

FColor GetEditorSuccessColor()
{
    return GetEditorColorToken(
        "Color.Editor.Success",
        IsLightEditorTheme() ? FColor::FromBytes(40, 168, 96) : FColor::FromBytes(125, 204, 138));
}

FColor GetEditorWarningColor()
{
    return GetEditorColorToken(
        "Color.Editor.Warning",
        IsLightEditorTheme() ? FColor::FromBytes(212, 132, 24) : FColor::FromBytes(230, 184, 104));
}

FColor GetEditorInspectorLabelColor()
{
    return GetEditorColorToken(
        "Color.Editor.Inspector.Label",
        IsLightEditorTheme() ? FColor::FromBytes(58, 64, 72) : FColor::FromBytes(224, 230, 237));
}

FColor GetEditorInspectorCompactLabelColor()
{
    return GetEditorColorToken(
        "Color.Editor.Inspector.CompactLabel",
        IsLightEditorTheme() ? FColor::FromBytes(120, 126, 136) : FColor::FromBytes(158, 168, 180));
}

FColor GetEditorTitleBarIconColor()
{
    return GetEditorColorToken(
        "Color.Editor.TitleBar.Icon",
        IsLightEditorTheme() ? FColor::FromBytes(60, 66, 74) : FColor::FromBytes(235, 242, 250));
}

FColor GetEditorTitleBarIconDisabledColor()
{
    return GetEditorTitleBarDisabledTextColor();
}

FColor GetEditorTreeIconColor()
{
    return GetEditorColorToken(
        "Color.Editor.Tree.Icon",
        IsLightEditorTheme() ? FColor::FromBytes(96, 104, 114) : FColor::FromBytes(214, 222, 234));
}

FButtonStyle MakeEditorTitleBarButtonStyle(bool bHighlighted)
{
    FButtonStyle style = FButtonStyle::CreatePrimary();
    const FColor textColor = GetEditorTitleBarTextColor();
    const FColor transparent = FColor::Transparent;
    const FColor baseColor = bHighlighted
        ? (IsLightEditorTheme() ? FColor::FromBytes(0, 120, 215, 34) : FColor::FromBytes(72, 104, 146, 116))
        : transparent;
    style.Normal = FButtonStateStyle(baseColor, transparent, textColor, 0.0f, 0.0f, false);
    style.Hovered = FButtonStateStyle(
        IsLightEditorTheme() ? FColor::FromBytes(0, 0, 0, 14) : FColor::FromBytes(255, 255, 255, 24),
        transparent,
        textColor,
        0.0f,
        0.0f,
        false);
    style.Pressed = FButtonStateStyle(
        IsLightEditorTheme() ? FColor::FromBytes(0, 0, 0, 24) : FColor::FromBytes(255, 255, 255, 38),
        transparent,
        textColor,
        0.0f,
        0.0f,
        false);
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

    if (IsLightEditorTheme()) {
        style.Normal = FButtonStateStyle(FColor::White, FColor::FromBytes(200, 200, 200), GetEditorPanelTitleColor(), 5.0f, 1.0f, false);
        style.Hovered = FButtonStateStyle(FColor::FromBytes(248, 248, 248), FColor::FromBytes(176, 176, 176), GetEditorPanelTitleColor(), 5.0f, 1.0f, false);
        style.Pressed = FButtonStateStyle(FColor::FromBytes(236, 236, 236), GetEditorAccentColor(), GetEditorPanelTitleColor(), 5.0f, 1.0f, false);
        style.Focused = style.Hovered;
        style.Disabled = FButtonStateStyle(FColor::FromBytes(244, 244, 244), FColor::FromBytes(214, 214, 214), FColor::FromBytes(150, 150, 150), 5.0f, 1.0f, false);
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
    style.Hovered = FButtonStateStyle(
        IsLightEditorTheme() ? FColor::FromBytes(0, 0, 0, 12) : FColor::FromBytes(255, 255, 255, 18),
        transparent,
        textColor,
        0.0f,
        0.0f,
        false);
    style.Pressed = FButtonStateStyle(
        IsLightEditorTheme() ? FColor::FromBytes(0, 0, 0, 22) : FColor::FromBytes(255, 255, 255, 30),
        transparent,
        textColor,
        0.0f,
        0.0f,
        false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(transparent, transparent, GetEditorTitleBarDisabledTextColor(), 0.0f, 0.0f, false);
    return style;
}

FButtonStyle MakeEditorPaletteItemButtonStyle(const FButtonStyle& baseStyle)
{
    FButtonStyle style = baseStyle;
    const FColor textColor = GetEditorPanelTitleColor();
    if (IsLightEditorTheme()) {
        style.Normal = FButtonStateStyle(FColor::White, FColor::FromBytes(214, 218, 224), textColor, 4.0f, 1.0f, false);
        style.Hovered = FButtonStateStyle(FColor::FromBytes(244, 247, 251), GetEditorAccentColor(), textColor, 4.0f, 1.0f, false);
        style.Pressed = FButtonStateStyle(FColor::FromBytes(232, 238, 246), GetEditorAccentColor(), textColor, 4.0f, 1.0f, false);
        style.Focused = style.Hovered;
        style.Disabled = FButtonStateStyle(FColor::FromBytes(242, 242, 242), FColor::FromBytes(220, 220, 220), FColor::FromBytes(150, 150, 150), 4.0f, 1.0f, false);
        return style;
    }

    const FColor background = FColor::FromBytes(34, 40, 49);
    const FColor hovered = FColor::FromBytes(43, 50, 61);
    const FColor pressed = FColor::FromBytes(52, 61, 74);
    const FColor selectedBorder = FColor::FromBytes(72, 104, 146);
    const FColor border = FColor::FromBytes(20, 24, 30);
    style.Normal = FButtonStateStyle(background, border, textColor, 4.0f, 1.0f, false);
    style.Hovered = FButtonStateStyle(hovered, selectedBorder, textColor, 4.0f, 1.0f, false);
    style.Pressed = FButtonStateStyle(pressed, GetEditorAccentColor(), textColor, 4.0f, 1.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(FColor::FromBytes(42, 47, 55), border, FColor::FromBytes(128, 136, 146), 4.0f, 1.0f, false);
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
    style.CornerRadius = 0.0f;
    style.BorderThickness = 1.0f;
    style.bDrawShadow = false;
    style.BackgroundColor = IsLightEditorTheme() ? FColor::White : GetEditorSurfaceAltBackgroundColor();
    style.BorderColor = IsLightEditorTheme() ? FColor::FromBytes(208, 212, 218) : FColor::FromBytes(16, 19, 23);
    return style;
}

FEditableTextStyle MakeEditorInspectorEditableTextStyle(const FEditableTextStyle& baseStyle)
{
    FEditableTextStyle style = baseStyle;
    if (IsLightEditorTheme()) {
        style.BackgroundColor = FColor::White;
        style.HoveredBackgroundColor = FColor::FromBytes(248, 248, 248);
        style.FocusedBackgroundColor = FColor::White;
        style.DisabledBackgroundColor = FColor::FromBytes(240, 240, 240);
        style.BorderColor = FColor::FromBytes(200, 200, 200);
        style.FocusedOutlineColor = GetEditorAccentColor();
        style.TextColor = FColor::FromBytes(30, 30, 30);
        style.DisabledTextColor = FColor::FromBytes(150, 150, 150);
        style.HintTextColor = FColor::FromBytes(132, 132, 132);
    } else {
        style.BackgroundColor = FColor::FromBytes(31, 37, 46);
        style.HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
        style.FocusedBackgroundColor = FColor::FromBytes(24, 31, 40);
        style.DisabledBackgroundColor = FColor::FromBytes(46, 52, 61);
        style.BorderColor = FColor::FromBytes(16, 19, 23);
        style.FocusedOutlineColor = GetEditorAccentColor();
        style.TextColor = FColor::FromBytes(238, 241, 245);
        style.DisabledTextColor = FColor::FromBytes(188, 196, 207);
        style.HintTextColor = FColor::FromBytes(135, 145, 157);
    }
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
    if (IsLightEditorTheme()) {
        style.BackgroundColor = FColor::White;
        style.HoveredBackgroundColor = FColor::FromBytes(248, 248, 248);
        style.PressedBackgroundColor = FColor::FromBytes(236, 236, 236);
        style.DisabledBackgroundColor = FColor::FromBytes(240, 240, 240);
        style.BorderColor = FColor::FromBytes(200, 200, 200);
        style.FocusedOutlineColor = GetEditorAccentColor();
        style.TextColor = FColor::FromBytes(30, 30, 30);
        style.PlaceholderTextColor = FColor::FromBytes(132, 132, 132);
        style.DisabledTextColor = FColor::FromBytes(150, 150, 150);
        style.ArrowColor = FColor::FromBytes(80, 80, 80);
        style.PopupRowHoveredColor = FColor::FromBytes(232, 240, 250);
        style.PopupRowSelectedColor = FColor::FromBytes(0, 120, 215);
        style.PopupRowSelectedHoveredColor = FColor::FromBytes(32, 138, 226);
        style.PopupOutlineColor = FColor::FromBytes(200, 200, 200);
    } else {
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
    }
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
    style.TextColor = IsLightEditorTheme() ? FColor::FromBytes(72, 78, 86) : FColor::FromBytes(188, 198, 210);
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
    style.BorderColor = IsLightEditorTheme() ? FColor::FromBytes(200, 200, 200) : FColor::FromBytes(16, 19, 23);
    style.FocusedOutlineColor = GetEditorAccentColor();
    style.TextColor = IsLightEditorTheme() ? FColor::FromBytes(58, 64, 72) : FColor::FromBytes(196, 205, 217);
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
    style.HoveredRowColor = IsLightEditorTheme() ? FColor::FromBytes(232, 238, 246) : FColor::FromBytes(43, 50, 61);
    style.SelectedRowColor = IsLightEditorTheme() ? FColor::FromBytes(214, 229, 247) : FColor::FromBytes(60, 86, 122);
    style.SelectedFocusedRowColor = IsLightEditorTheme() ? FColor::FromBytes(0, 120, 215) : FColor::FromBytes(74, 106, 150);
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
    style.HoveredRowColor = IsLightEditorTheme() ? FColor::FromBytes(232, 238, 246) : FColor::FromBytes(43, 50, 61);
    style.SelectedRowColor = IsLightEditorTheme() ? FColor::FromBytes(214, 229, 247) : FColor::FromBytes(60, 86, 122);
    style.SelectedFocusedRowColor = IsLightEditorTheme() ? FColor::FromBytes(0, 120, 215) : FColor::FromBytes(74, 106, 150);
    style.IndicatorColor = IsLightEditorTheme() ? FColor::FromBytes(90, 98, 108) : FColor::FromBytes(228, 232, 238);
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
    if (IsLightEditorTheme()) {
        style.TabColor = FColor::FromBytes(230, 233, 238);
        style.TabHoveredColor = FColor::FromBytes(220, 226, 234);
        style.TabPressedColor = FColor::FromBytes(212, 219, 228);
        style.ActiveTabColor = FColor::White;
        style.TextColor = FColor::FromBytes(72, 78, 86);
        style.ActiveTextColor = FColor::FromBytes(20, 20, 20);
        style.DisabledTextColor = FColor::FromBytes(150, 150, 150);
    } else {
        style.TabColor = FColor::FromBytes(33, 39, 47);
        style.TabHoveredColor = FColor::FromBytes(43, 50, 60);
        style.TabPressedColor = FColor::FromBytes(28, 34, 41);
        style.ActiveTabColor = FColor::FromBytes(67, 96, 138);
        style.TextColor = FColor::FromBytes(180, 189, 201);
        style.ActiveTextColor = FColor::FromBytes(241, 246, 252);
        style.DisabledTextColor = FColor::FromBytes(118, 126, 137);
    }
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
    if (IsLightEditorTheme()) {
        style.TabColor = FColor::FromBytes(230, 233, 238);
        style.TabHoveredColor = FColor::FromBytes(220, 226, 234);
        style.TabPressedColor = FColor::FromBytes(212, 219, 228);
        style.ActiveTabColor = FColor::White;
        style.TextColor = FColor::FromBytes(72, 78, 86);
        style.ActiveTextColor = FColor::FromBytes(20, 20, 20);
    } else {
        style.TabColor = FColor::FromBytes(34, 40, 49);
        style.TabHoveredColor = FColor::FromBytes(44, 51, 62);
        style.TabPressedColor = FColor::FromBytes(30, 35, 43);
        style.ActiveTabColor = FColor::FromBytes(67, 96, 138);
        style.TextColor = FColor::FromBytes(176, 186, 198);
        style.ActiveTextColor = FColor::FromBytes(241, 246, 252);
    }
    return style;
}

FTitleBarStyle MakeEditorTitleBarStyle(const FTitleBarStyle& baseStyle)
{
    FTitleBarStyle style = baseStyle;
    style.Height = 24.0f;
    style.Padding = FMargin(4.0f, 0.0f, 0.0f, 0.0f);
    style.ItemSpacing = 4.0f;
    style.BackgroundColor = IsLightEditorTheme()
        ? FColor::FromBytes(246, 247, 249)
        : FColor::FromBytes(24, 29, 36);
    style.BorderColor = IsLightEditorTheme()
        ? FColor::FromBytes(210, 214, 220)
        : FColor::FromBytes(16, 19, 23);
    style.BorderThickness = 1.0f;
    style.SystemButtonSize = 34.0f;
    style.SystemButtonGlyphColor = GetEditorTitleBarTextColor();
    style.HoveredSystemButtonColor = IsLightEditorTheme()
        ? FColor::FromBytes(0, 0, 0, 18)
        : FColor::FromBytes(255, 255, 255, 24);
    style.PressedSystemButtonColor = IsLightEditorTheme()
        ? FColor::FromBytes(0, 0, 0, 30)
        : FColor::FromBytes(255, 255, 255, 40);
    style.CloseButtonHoveredColor = FColor::FromBytes(212, 58, 76, 224);
    style.CloseButtonPressedColor = FColor::FromBytes(188, 46, 66, 240);
    style.MinDesiredSize = FVector2(0.0f, 24.0f);
    return style;
}

FHorizontalSplitterStyle MakeEditorHorizontalSplitterStyle(const FHorizontalSplitterStyle& baseStyle)
{
    FHorizontalSplitterStyle style = baseStyle;
    style.BarWidth = 5.0f;
    style.Color = IsLightEditorTheme() ? FColor::FromBytes(196, 202, 210) : FColor::FromBytes(44, 51, 61);
    style.HoveredColor = IsLightEditorTheme() ? FColor::FromBytes(166, 175, 186) : FColor::FromBytes(70, 82, 99);
    style.ActiveColor = GetEditorAccentColor();
    return style;
}

FVerticalSplitterStyle MakeEditorVerticalSplitterStyle(const FVerticalSplitterStyle& baseStyle)
{
    FVerticalSplitterStyle style = baseStyle;
    style.BarHeight = 5.0f;
    style.Color = IsLightEditorTheme() ? FColor::FromBytes(196, 202, 210) : FColor::FromBytes(44, 51, 61);
    style.HoveredColor = IsLightEditorTheme() ? FColor::FromBytes(166, 175, 186) : FColor::FromBytes(70, 82, 99);
    style.ActiveColor = GetEditorAccentColor();
    return style;
}

} // namespace ImWidgetV4Editor
