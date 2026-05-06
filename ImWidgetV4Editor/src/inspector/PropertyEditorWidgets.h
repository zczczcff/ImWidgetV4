#pragma once

#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <memory>
#include <string>

namespace ImWidgetV4Editor::PropertyEditorWidgets {

using namespace ImWidgetV4;

inline FEditableTextStyle MakeInspectorEditableTextStyle()
{
    FEditableTextStyle style;
    style.BackgroundColor = FColor::FromBytes(31, 37, 46);
    style.HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
    style.FocusedBackgroundColor = FColor::FromBytes(24, 31, 40);
    style.DisabledBackgroundColor = FColor::FromBytes(46, 52, 61);
    style.BorderColor = FColor::FromBytes(16, 19, 23);
    style.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
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

inline FComboBoxStyle MakeInspectorComboBoxStyle()
{
    FComboBoxStyle style;
    style.BackgroundColor = FColor::FromBytes(31, 37, 46);
    style.HoveredBackgroundColor = FColor::FromBytes(39, 46, 56);
    style.PressedBackgroundColor = FColor::FromBytes(24, 31, 40);
    style.DisabledBackgroundColor = FColor::FromBytes(46, 52, 61);
    style.BorderColor = FColor::FromBytes(16, 19, 23);
    style.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
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

inline void ApplyInspectorEditableTextStyle(ImEditableText& editor, bool bReadOnly)
{
    editor.SetStyle(MakeInspectorEditableTextStyle());
    editor.SetDisabled(bReadOnly);
}

inline void ApplyInspectorComboBoxStyle(ImComboBox& comboBox)
{
    comboBox.SetStyle(MakeInspectorComboBoxStyle());
}

inline std::shared_ptr<ImTextBlock> MakeInspectorPropertyLabel(const std::string& text)
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(text + ":");
    label->SetWrapText(false);
    label->SetFontSize(13.0f);
    label->SetTextColor(FColor::FromBytes(224, 230, 237));
    return label;
}

inline std::shared_ptr<ImEditableText> MakeInspectorReadOnlyField(const std::string& text)
{
    auto field = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*field, true);
    field->SetText(text);
    return field;
}

inline std::shared_ptr<ImHorizontalBox> MakeInspectorPropertyRow(
    const std::string& labelText,
    const std::shared_ptr<ImWidget>& valueWidget)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);
    row->AddChild(MakeInspectorPropertyLabel(labelText), FMargin(2.0f));
    row->AddChildFill(valueWidget, 1.0f, FMargin(2.0f));
    return row;
}

} // namespace ImWidgetV4Editor::PropertyEditorWidgets
