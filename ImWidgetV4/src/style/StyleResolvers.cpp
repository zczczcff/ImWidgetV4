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
