#include <imwidgetv4/style/StyleResolvers.h>

namespace ImWidgetV4 {

namespace {

void ApplyButtonStateStyle(
    FButtonStateStyle& state,
    const FStyleSet& styleSet,
    const char* colorPrefix,
    const char* floatPrefix)
{
    state.BackgroundColor = styleSet.GetColor(
        std::string(colorPrefix) + ".Background",
        state.BackgroundColor);
    state.BorderColor = styleSet.GetColor(
        std::string(colorPrefix) + ".Border",
        state.BorderColor);
    state.TextColor = styleSet.GetColor(
        std::string(colorPrefix) + ".Text",
        state.TextColor);
    state.BorderThickness = styleSet.GetFloat(
        std::string(floatPrefix) + ".BorderThickness",
        state.BorderThickness);
    state.CornerRadius = styleSet.GetFloat(
        std::string(floatPrefix) + ".CornerRadius",
        state.CornerRadius);
}

} // namespace

FButtonStyle ResolveButtonStyle(const FStyleSet& styleSet)
{
    FButtonStyle style;

    ApplyButtonStateStyle(style.Normal, styleSet, "Color.Button.Normal", "Float.Button.Normal");
    ApplyButtonStateStyle(style.Hovered, styleSet, "Color.Button.Hovered", "Float.Button.Hovered");
    ApplyButtonStateStyle(style.Pressed, styleSet, "Color.Button.Pressed", "Float.Button.Pressed");
    ApplyButtonStateStyle(style.Focused, styleSet, "Color.Button.Focused", "Float.Button.Focused");
    ApplyButtonStateStyle(style.Disabled, styleSet, "Color.Button.Disabled", "Float.Button.Disabled");

    const float sharedBorderThickness = styleSet.GetFloat("Float.Button.BorderThickness", style.Normal.BorderThickness);
    const float sharedCornerRadius = styleSet.GetFloat("Float.Button.CornerRadius", style.Normal.CornerRadius);

    style.Normal.BorderThickness = styleSet.GetFloat("Float.Button.Normal.BorderThickness", sharedBorderThickness);
    style.Hovered.BorderThickness = styleSet.GetFloat("Float.Button.Hovered.BorderThickness", sharedBorderThickness);
    style.Pressed.BorderThickness = styleSet.GetFloat("Float.Button.Pressed.BorderThickness", sharedBorderThickness);
    style.Focused.BorderThickness = styleSet.GetFloat("Float.Button.Focused.BorderThickness", sharedBorderThickness);
    style.Disabled.BorderThickness = styleSet.GetFloat("Float.Button.Disabled.BorderThickness", sharedBorderThickness);

    style.Normal.CornerRadius = styleSet.GetFloat("Float.Button.Normal.CornerRadius", sharedCornerRadius);
    style.Hovered.CornerRadius = styleSet.GetFloat("Float.Button.Hovered.CornerRadius", sharedCornerRadius);
    style.Pressed.CornerRadius = styleSet.GetFloat("Float.Button.Pressed.CornerRadius", sharedCornerRadius);
    style.Focused.CornerRadius = styleSet.GetFloat("Float.Button.Focused.CornerRadius", sharedCornerRadius);
    style.Disabled.CornerRadius = styleSet.GetFloat("Float.Button.Disabled.CornerRadius", sharedCornerRadius);

    return style;
}

FCheckBoxStyle ResolveCheckBoxStyle(const FStyleSet& styleSet)
{
    FCheckBoxStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.CheckBox.Background", style.BackgroundColor);
    style.HoveredBackgroundColor = styleSet.GetColor(
        "Color.CheckBox.HoveredBackground",
        style.HoveredBackgroundColor);
    style.PressedBackgroundColor = styleSet.GetColor(
        "Color.CheckBox.PressedBackground",
        style.PressedBackgroundColor);
    style.CheckedBackgroundColor = styleSet.GetColor(
        "Color.CheckBox.CheckedBackground",
        style.CheckedBackgroundColor);
    style.DisabledBackgroundColor = styleSet.GetColor(
        "Color.CheckBox.DisabledBackground",
        style.DisabledBackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.CheckBox.Border", style.BorderColor);
    style.CheckMarkColor = styleSet.GetColor("Color.CheckBox.CheckMark", style.CheckMarkColor);
    style.TextColor = styleSet.GetColor("Color.CheckBox.Text", style.TextColor);
    style.DisabledTextColor = styleSet.GetColor("Color.CheckBox.DisabledText", style.DisabledTextColor);
    style.FocusedOutlineColor = styleSet.GetColor(
        "Color.CheckBox.FocusedOutline",
        style.FocusedOutlineColor);
    style.IndicatorSize = styleSet.GetFloat("Float.CheckBox.IndicatorSize", style.IndicatorSize);
    style.IndicatorCornerRadius = styleSet.GetFloat(
        "Float.CheckBox.IndicatorCornerRadius",
        style.IndicatorCornerRadius);
    style.LabelSpacing = styleSet.GetFloat("Float.CheckBox.LabelSpacing", style.LabelSpacing);
    style.BorderThickness = styleSet.GetFloat("Float.CheckBox.BorderThickness", style.BorderThickness);
    style.FontSize = styleSet.GetFloat("Float.CheckBox.FontSize", style.FontSize);

    return style;
}

FComboBoxStyle ResolveComboBoxStyle(const FStyleSet& styleSet)
{
    FComboBoxStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.ComboBox.Background", style.BackgroundColor);
    style.HoveredBackgroundColor = styleSet.GetColor(
        "Color.ComboBox.HoveredBackground",
        style.HoveredBackgroundColor);
    style.PressedBackgroundColor = styleSet.GetColor(
        "Color.ComboBox.PressedBackground",
        style.PressedBackgroundColor);
    style.DisabledBackgroundColor = styleSet.GetColor(
        "Color.ComboBox.DisabledBackground",
        style.DisabledBackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.ComboBox.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor(
        "Color.ComboBox.FocusedOutline",
        style.FocusedOutlineColor);
    style.TextColor = styleSet.GetColor("Color.ComboBox.Text", style.TextColor);
    style.PlaceholderTextColor = styleSet.GetColor(
        "Color.ComboBox.PlaceholderText",
        style.PlaceholderTextColor);
    style.DisabledTextColor = styleSet.GetColor(
        "Color.ComboBox.DisabledText",
        style.DisabledTextColor);
    style.ArrowColor = styleSet.GetColor("Color.ComboBox.Arrow", style.ArrowColor);
    style.PopupRowHoveredColor = styleSet.GetColor(
        "Color.ComboBox.PopupRowHovered",
        style.PopupRowHoveredColor);
    style.PopupRowSelectedColor = styleSet.GetColor(
        "Color.ComboBox.PopupRowSelected",
        style.PopupRowSelectedColor);
    style.PopupRowSelectedHoveredColor = styleSet.GetColor(
        "Color.ComboBox.PopupRowSelectedHovered",
        style.PopupRowSelectedHoveredColor);
    style.PopupOutlineColor = styleSet.GetColor(
        "Color.ComboBox.PopupOutline",
        style.PopupOutlineColor);
    style.FontSize = styleSet.GetFloat("Float.ComboBox.FontSize", style.FontSize);
    style.BorderThickness = styleSet.GetFloat("Float.ComboBox.BorderThickness", style.BorderThickness);
    style.CornerRadius = styleSet.GetFloat("Float.ComboBox.CornerRadius", style.CornerRadius);
    style.ArrowSize = styleSet.GetFloat("Float.ComboBox.ArrowSize", style.ArrowSize);
    style.PopupItemHeight = styleSet.GetFloat("Float.ComboBox.PopupItemHeight", style.PopupItemHeight);
    style.PopupMaxVisibleItems = styleSet.GetFloat(
        "Float.ComboBox.PopupMaxVisibleItems",
        style.PopupMaxVisibleItems);

    return style;
}

FEditableTextStyle ResolveEditableTextStyle(const FStyleSet& styleSet)
{
    FEditableTextStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.Input.Background", style.BackgroundColor);
    style.HoveredBackgroundColor = styleSet.GetColor("Color.Input.HoveredBackground", style.HoveredBackgroundColor);
    style.FocusedBackgroundColor = styleSet.GetColor("Color.Input.FocusedBackground", style.FocusedBackgroundColor);
    style.DisabledBackgroundColor = styleSet.GetColor("Color.Input.DisabledBackground", style.DisabledBackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.Input.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor("Color.Input.FocusedOutline", style.FocusedOutlineColor);
    style.TextColor = styleSet.GetColor("Color.Input.Text", style.TextColor);
    style.DisabledTextColor = styleSet.GetColor("Color.Input.DisabledText", style.DisabledTextColor);
    style.HintTextColor = styleSet.GetColor("Color.Input.HintText", style.HintTextColor);
    style.CaretColor = styleSet.GetColor("Color.Input.Caret", style.CaretColor);
    style.SelectionBackgroundColor = styleSet.GetColor(
        "Color.Input.SelectionBackground",
        style.SelectionBackgroundColor);
    style.SelectedTextColor = styleSet.GetColor("Color.Input.SelectedText", style.SelectedTextColor);
    style.CornerRadius = styleSet.GetFloat("Float.Input.CornerRadius", style.CornerRadius);
    style.BorderThickness = styleSet.GetFloat("Float.Input.BorderThickness", style.BorderThickness);
    style.FontSize = styleSet.GetFloat("Float.Input.FontSize", style.FontSize);

    return style;
}

FListViewStyle ResolveListViewStyle(const FStyleSet& styleSet)
{
    FListViewStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.ListView.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.ListView.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor("Color.ListView.FocusedOutline", style.FocusedOutlineColor);
    style.HoveredRowColor = styleSet.GetColor("Color.ListView.HoveredRow", style.HoveredRowColor);
    style.SelectedRowColor = styleSet.GetColor("Color.ListView.SelectedRow", style.SelectedRowColor);
    style.SelectedFocusedRowColor = styleSet.GetColor(
        "Color.ListView.SelectedFocusedRow",
        style.SelectedFocusedRowColor);
    style.ScrollbarTrackColor = styleSet.GetColor(
        "Color.ListView.ScrollbarTrack",
        style.ScrollbarTrackColor);
    style.ScrollbarThumbColor = styleSet.GetColor(
        "Color.ListView.ScrollbarThumb",
        style.ScrollbarThumbColor);
    style.ScrollbarThumbHoveredColor = styleSet.GetColor(
        "Color.ListView.ScrollbarThumbHovered",
        style.ScrollbarThumbHoveredColor);
    style.CornerRadius = styleSet.GetFloat("Float.ListView.CornerRadius", style.CornerRadius);
    style.BorderThickness = styleSet.GetFloat("Float.ListView.BorderThickness", style.BorderThickness);
    style.RowMinHeight = styleSet.GetFloat("Float.ListView.RowMinHeight", style.RowMinHeight);
    style.ScrollbarThickness = styleSet.GetFloat("Float.ListView.ScrollbarThickness", style.ScrollbarThickness);
    style.ScrollbarPadding = styleSet.GetFloat("Float.ListView.ScrollbarPadding", style.ScrollbarPadding);
    style.ThumbMinLength = styleSet.GetFloat("Float.ListView.ThumbMinLength", style.ThumbMinLength);
    style.WheelScrollStep = styleSet.GetFloat("Float.ListView.WheelScrollStep", style.WheelScrollStep);

    return style;
}

FOutlineViewStyle ResolveOutlineViewStyle(const FStyleSet& styleSet)
{
    FOutlineViewStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.OutlineView.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.OutlineView.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor(
        "Color.OutlineView.FocusedOutline",
        style.FocusedOutlineColor);
    style.HoveredRowColor = styleSet.GetColor("Color.OutlineView.HoveredRow", style.HoveredRowColor);
    style.SelectedRowColor = styleSet.GetColor("Color.OutlineView.SelectedRow", style.SelectedRowColor);
    style.SelectedFocusedRowColor = styleSet.GetColor(
        "Color.OutlineView.SelectedFocusedRow",
        style.SelectedFocusedRowColor);
    style.IndicatorColor = styleSet.GetColor("Color.OutlineView.Indicator", style.IndicatorColor);
    style.ScrollbarTrackColor = styleSet.GetColor(
        "Color.OutlineView.ScrollbarTrack",
        style.ScrollbarTrackColor);
    style.ScrollbarThumbColor = styleSet.GetColor(
        "Color.OutlineView.ScrollbarThumb",
        style.ScrollbarThumbColor);
    style.ScrollbarThumbHoveredColor = styleSet.GetColor(
        "Color.OutlineView.ScrollbarThumbHovered",
        style.ScrollbarThumbHoveredColor);
    style.CornerRadius = styleSet.GetFloat("Float.OutlineView.CornerRadius", style.CornerRadius);
    style.BorderThickness = styleSet.GetFloat("Float.OutlineView.BorderThickness", style.BorderThickness);
    style.IndentWidth = styleSet.GetFloat("Float.OutlineView.IndentWidth", style.IndentWidth);
    style.IndicatorSize = styleSet.GetFloat("Float.OutlineView.IndicatorSize", style.IndicatorSize);
    style.IndicatorSpacing = styleSet.GetFloat("Float.OutlineView.IndicatorSpacing", style.IndicatorSpacing);
    style.RowMinHeight = styleSet.GetFloat("Float.OutlineView.RowMinHeight", style.RowMinHeight);
    style.ScrollbarThickness = styleSet.GetFloat(
        "Float.OutlineView.ScrollbarThickness",
        style.ScrollbarThickness);
    style.ScrollbarPadding = styleSet.GetFloat("Float.OutlineView.ScrollbarPadding", style.ScrollbarPadding);
    style.ThumbMinLength = styleSet.GetFloat("Float.OutlineView.ThumbMinLength", style.ThumbMinLength);
    style.WheelScrollStep = styleSet.GetFloat("Float.OutlineView.WheelScrollStep", style.WheelScrollStep);

    return style;
}

FPopupMenuStyle ResolvePopupMenuStyle(const FStyleSet& styleSet)
{
    FPopupMenuStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.PopupMenu.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.PopupMenu.Border", style.BorderColor);
    style.RowHoveredColor = styleSet.GetColor("Color.PopupMenu.RowHovered", style.RowHoveredColor);
    style.RowPressedColor = styleSet.GetColor("Color.PopupMenu.RowPressed", style.RowPressedColor);
    style.TextColor = styleSet.GetColor("Color.PopupMenu.Text", style.TextColor);
    style.DisabledTextColor = styleSet.GetColor(
        "Color.PopupMenu.DisabledText",
        style.DisabledTextColor);
    style.SeparatorColor = styleSet.GetColor("Color.PopupMenu.Separator", style.SeparatorColor);
    style.SubmenuArrowColor = styleSet.GetColor(
        "Color.PopupMenu.SubmenuArrow",
        style.SubmenuArrowColor);
    style.FontSize = styleSet.GetFloat("Float.PopupMenu.FontSize", style.FontSize);
    style.RowHeight = styleSet.GetFloat("Float.PopupMenu.RowHeight", style.RowHeight);
    style.IconSize = styleSet.GetFloat("Float.PopupMenu.IconSize", style.IconSize);
    style.HorizontalPadding = styleSet.GetFloat(
        "Float.PopupMenu.HorizontalPadding",
        style.HorizontalPadding);
    style.IconTextSpacing = styleSet.GetFloat(
        "Float.PopupMenu.IconTextSpacing",
        style.IconTextSpacing);
    style.SubmenuIndicatorSpacing = styleSet.GetFloat(
        "Float.PopupMenu.SubmenuIndicatorSpacing",
        style.SubmenuIndicatorSpacing);
    style.OuterPaddingX = styleSet.GetFloat("Float.PopupMenu.OuterPaddingX", style.OuterPaddingX);
    style.OuterPaddingY = styleSet.GetFloat("Float.PopupMenu.OuterPaddingY", style.OuterPaddingY);
    style.CornerRadius = styleSet.GetFloat("Float.PopupMenu.CornerRadius", style.CornerRadius);
    style.BorderThickness = styleSet.GetFloat("Float.PopupMenu.BorderThickness", style.BorderThickness);

    return style;
}

FScrollBoxStyle ResolveScrollBoxStyle(const FStyleSet& styleSet)
{
    FScrollBoxStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.ScrollBox.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.ScrollBox.Border", style.BorderColor);
    style.ScrollbarTrackColor = styleSet.GetColor(
        "Color.ScrollBox.ScrollbarTrack",
        style.ScrollbarTrackColor);
    style.ScrollbarThumbColor = styleSet.GetColor(
        "Color.ScrollBox.ScrollbarThumb",
        style.ScrollbarThumbColor);
    style.ScrollbarThumbHoveredColor = styleSet.GetColor(
        "Color.ScrollBox.ScrollbarThumbHovered",
        style.ScrollbarThumbHoveredColor);
    style.BorderThickness = styleSet.GetFloat("Float.ScrollBox.BorderThickness", style.BorderThickness);
    style.CornerRadius = styleSet.GetFloat("Float.ScrollBox.CornerRadius", style.CornerRadius);
    style.ScrollbarThickness = styleSet.GetFloat(
        "Float.ScrollBox.ScrollbarThickness",
        style.ScrollbarThickness);
    style.ScrollbarPadding = styleSet.GetFloat(
        "Float.ScrollBox.ScrollbarPadding",
        style.ScrollbarPadding);
    style.ThumbMinLength = styleSet.GetFloat("Float.ScrollBox.ThumbMinLength", style.ThumbMinLength);
    style.WheelScrollStep = styleSet.GetFloat("Float.ScrollBox.WheelScrollStep", style.WheelScrollStep);

    return style;
}

FSwitchStyle ResolveSwitchStyle(const FStyleSet& styleSet)
{
    FSwitchStyle style;

    style.OffTrackColor = styleSet.GetColor("Color.Switch.OffTrack", style.OffTrackColor);
    style.OffTrackHoveredColor = styleSet.GetColor(
        "Color.Switch.OffTrackHovered",
        style.OffTrackHoveredColor);
    style.OffTrackPressedColor = styleSet.GetColor(
        "Color.Switch.OffTrackPressed",
        style.OffTrackPressedColor);
    style.OnTrackColor = styleSet.GetColor("Color.Switch.OnTrack", style.OnTrackColor);
    style.OnTrackHoveredColor = styleSet.GetColor(
        "Color.Switch.OnTrackHovered",
        style.OnTrackHoveredColor);
    style.OnTrackPressedColor = styleSet.GetColor(
        "Color.Switch.OnTrackPressed",
        style.OnTrackPressedColor);
    style.DisabledTrackColor = styleSet.GetColor(
        "Color.Switch.DisabledTrack",
        style.DisabledTrackColor);
    style.ThumbColor = styleSet.GetColor("Color.Switch.Thumb", style.ThumbColor);
    style.ThumbHoveredColor = styleSet.GetColor(
        "Color.Switch.ThumbHovered",
        style.ThumbHoveredColor);
    style.ThumbPressedColor = styleSet.GetColor(
        "Color.Switch.ThumbPressed",
        style.ThumbPressedColor);
    style.DisabledThumbColor = styleSet.GetColor(
        "Color.Switch.DisabledThumb",
        style.DisabledThumbColor);
    style.BorderColor = styleSet.GetColor("Color.Switch.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor(
        "Color.Switch.FocusedOutline",
        style.FocusedOutlineColor);
    style.BorderThickness = styleSet.GetFloat("Float.Switch.BorderThickness", style.BorderThickness);
    style.ThumbInset = styleSet.GetFloat("Float.Switch.ThumbInset", style.ThumbInset);
    style.DesiredSize = styleSet.GetVector2("Vector2.Switch.DesiredSize", style.DesiredSize);

    return style;
}

FTextOutlineViewStyle ResolveTextOutlineViewStyle(const FStyleSet& styleSet)
{
    FTextOutlineViewStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.TextOutlineView.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.TextOutlineView.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor(
        "Color.TextOutlineView.FocusedOutline",
        style.FocusedOutlineColor);
    style.TextColor = styleSet.GetColor("Color.TextOutlineView.Text", style.TextColor);
    style.HoveredRowColor = styleSet.GetColor("Color.TextOutlineView.HoveredRow", style.HoveredRowColor);
    style.SelectedRowColor = styleSet.GetColor("Color.TextOutlineView.SelectedRow", style.SelectedRowColor);
    style.SelectedFocusedRowColor = styleSet.GetColor(
        "Color.TextOutlineView.SelectedFocusedRow",
        style.SelectedFocusedRowColor);
    style.IndicatorColor = styleSet.GetColor("Color.TextOutlineView.Indicator", style.IndicatorColor);
    style.ScrollbarTrackColor = styleSet.GetColor(
        "Color.TextOutlineView.ScrollbarTrack",
        style.ScrollbarTrackColor);
    style.ScrollbarThumbColor = styleSet.GetColor(
        "Color.TextOutlineView.ScrollbarThumb",
        style.ScrollbarThumbColor);
    style.ScrollbarThumbHoveredColor = styleSet.GetColor(
        "Color.TextOutlineView.ScrollbarThumbHovered",
        style.ScrollbarThumbHoveredColor);
    style.CornerRadius = styleSet.GetFloat("Float.TextOutlineView.CornerRadius", style.CornerRadius);
    style.BorderThickness = styleSet.GetFloat("Float.TextOutlineView.BorderThickness", style.BorderThickness);
    style.FontSize = styleSet.GetFloat("Float.TextOutlineView.FontSize", style.FontSize);
    style.RowHeight = styleSet.GetFloat("Float.TextOutlineView.RowHeight", style.RowHeight);
    style.IndentWidth = styleSet.GetFloat("Float.TextOutlineView.IndentWidth", style.IndentWidth);
    style.IndicatorSize = styleSet.GetFloat("Float.TextOutlineView.IndicatorSize", style.IndicatorSize);
    style.IndicatorSpacing = styleSet.GetFloat("Float.TextOutlineView.IndicatorSpacing", style.IndicatorSpacing);
    style.IconSize = styleSet.GetFloat("Float.TextOutlineView.IconSize", style.IconSize);
    style.IconSpacing = styleSet.GetFloat("Float.TextOutlineView.IconSpacing", style.IconSpacing);
    style.ScrollbarThickness = styleSet.GetFloat(
        "Float.TextOutlineView.ScrollbarThickness",
        style.ScrollbarThickness);
    style.ScrollbarPadding = styleSet.GetFloat(
        "Float.TextOutlineView.ScrollbarPadding",
        style.ScrollbarPadding);
    style.ThumbMinLength = styleSet.GetFloat("Float.TextOutlineView.ThumbMinLength", style.ThumbMinLength);
    style.WheelScrollStep = styleSet.GetFloat("Float.TextOutlineView.WheelScrollStep", style.WheelScrollStep);

    return style;
}

FTabViewStyle ResolveTabViewStyle(const FStyleSet& styleSet)
{
    FTabViewStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.TabView.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.TabView.Border", style.BorderColor);
    style.FocusedOutlineColor = styleSet.GetColor(
        "Color.TabView.FocusedOutline",
        style.FocusedOutlineColor);
    style.TabStripBackgroundColor = styleSet.GetColor(
        "Color.TabView.TabStripBackground",
        style.TabStripBackgroundColor);
    style.TabColor = styleSet.GetColor("Color.TabView.Tab", style.TabColor);
    style.TabHoveredColor = styleSet.GetColor("Color.TabView.TabHovered", style.TabHoveredColor);
    style.TabPressedColor = styleSet.GetColor("Color.TabView.TabPressed", style.TabPressedColor);
    style.ActiveTabColor = styleSet.GetColor("Color.TabView.ActiveTab", style.ActiveTabColor);
    style.DisabledTabColor = styleSet.GetColor("Color.TabView.DisabledTab", style.DisabledTabColor);
    style.TextColor = styleSet.GetColor("Color.TabView.Text", style.TextColor);
    style.ActiveTextColor = styleSet.GetColor("Color.TabView.ActiveText", style.ActiveTextColor);
    style.DisabledTextColor = styleSet.GetColor("Color.TabView.DisabledText", style.DisabledTextColor);
    style.TabBorderColor = styleSet.GetColor("Color.TabView.TabBorder", style.TabBorderColor);
    style.DirtyMarkerColor = styleSet.GetColor("Color.TabView.DirtyMarker", style.DirtyMarkerColor);
    style.CloseButtonColor = styleSet.GetColor("Color.TabView.CloseButton", style.CloseButtonColor);
    style.CloseButtonHoveredColor = styleSet.GetColor(
        "Color.TabView.CloseButtonHovered",
        style.CloseButtonHoveredColor);
    style.CloseButtonPressedColor = styleSet.GetColor(
        "Color.TabView.CloseButtonPressed",
        style.CloseButtonPressedColor);
    style.OverflowButtonColor = styleSet.GetColor(
        "Color.TabView.OverflowButton",
        style.OverflowButtonColor);
    style.OverflowButtonHoveredColor = styleSet.GetColor(
        "Color.TabView.OverflowButtonHovered",
        style.OverflowButtonHoveredColor);
    style.OverflowButtonPressedColor = styleSet.GetColor(
        "Color.TabView.OverflowButtonPressed",
        style.OverflowButtonPressedColor);
    style.OverflowButtonDisabledColor = styleSet.GetColor(
        "Color.TabView.OverflowButtonDisabled",
        style.OverflowButtonDisabledColor);
    style.TabSpacing = styleSet.GetFloat("Float.TabView.TabSpacing", style.TabSpacing);
    style.TabMinWidth = styleSet.GetFloat("Float.TabView.TabMinWidth", style.TabMinWidth);
    style.TabHeight = styleSet.GetFloat("Float.TabView.TabHeight", style.TabHeight);
    style.IconSize = styleSet.GetFloat("Float.TabView.IconSize", style.IconSize);
    style.DirtyMarkerRadius = styleSet.GetFloat(
        "Float.TabView.DirtyMarkerRadius",
        style.DirtyMarkerRadius);
    style.CloseButtonSize = styleSet.GetFloat("Float.TabView.CloseButtonSize", style.CloseButtonSize);
    style.OverflowButtonWidth = styleSet.GetFloat(
        "Float.TabView.OverflowButtonWidth",
        style.OverflowButtonWidth);
    style.FontSize = styleSet.GetFloat("Float.TabView.FontSize", style.FontSize);
    style.BorderThickness = styleSet.GetFloat("Float.TabView.BorderThickness", style.BorderThickness);
    style.CornerRadius = styleSet.GetFloat("Float.TabView.CornerRadius", style.CornerRadius);

    return style;
}

FTitleBarStyle ResolveTitleBarStyle(const FStyleSet& styleSet)
{
    FTitleBarStyle style;

    style.BackgroundColor = styleSet.GetColor("Color.TitleBar.Background", style.BackgroundColor);
    style.BorderColor = styleSet.GetColor("Color.TitleBar.Border", style.BorderColor);
    style.HoveredSystemButtonColor = styleSet.GetColor(
        "Color.TitleBar.SystemButton.Hovered",
        style.HoveredSystemButtonColor);
    style.PressedSystemButtonColor = styleSet.GetColor(
        "Color.TitleBar.SystemButton.Pressed",
        style.PressedSystemButtonColor);
    style.CloseButtonHoveredColor = styleSet.GetColor(
        "Color.TitleBar.CloseButton.Hovered",
        style.CloseButtonHoveredColor);
    style.CloseButtonPressedColor = styleSet.GetColor(
        "Color.TitleBar.CloseButton.Pressed",
        style.CloseButtonPressedColor);
    style.Height = styleSet.GetFloat("Float.TitleBar.Height", style.Height);
    style.DragRegionMinWidth = styleSet.GetFloat(
        "Float.TitleBar.DragRegionMinWidth",
        style.DragRegionMinWidth);
    style.SystemButtonSize = styleSet.GetFloat("Float.TitleBar.SystemButtonSize", style.SystemButtonSize);
    style.SystemButtonSpacing = styleSet.GetFloat(
        "Float.TitleBar.SystemButtonSpacing",
        style.SystemButtonSpacing);

    return style;
}

} // namespace ImWidgetV4
