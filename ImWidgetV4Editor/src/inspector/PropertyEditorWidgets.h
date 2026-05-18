#pragma once

#include "../editor/EditorTheme.h"
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>
#include <string>

namespace ImWidgetV4Editor::PropertyEditorWidgets {

using namespace ImWidgetV4;

inline FEditableTextStyle MakeInspectorEditableTextStyle()
{
    return MakeEditorInspectorEditableTextStyle();
}

inline FComboBoxStyle MakeInspectorComboBoxStyle()
{
    return MakeEditorInspectorComboBoxStyle();
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

inline FSwitchStyle MakeInspectorSwitchStyle()
{
    return MakeEditorInspectorSwitchStyle();
}

inline void ApplyInspectorSwitchStyle(ImSwitch& switchWidget)
{
    switchWidget.SetStyle(MakeInspectorSwitchStyle());
}

inline std::shared_ptr<ImWidget> MakeInspectorFlexibleSpacer()
{
    auto spacer = std::make_shared<ImWidget>();
    spacer->SetHitTestVisible(false);
    return spacer;
}

inline std::shared_ptr<ImTextBlock> MakeInspectorPropertyLabel(const std::string& text)
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(text + ":");
    label->SetWrapText(false);
    label->SetFontSize(13.0f);
    label->SetTextColor(GetEditorInspectorLabelColor());
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
    if (!labelText.empty()) {
        row->AddChild(MakeInspectorPropertyLabel(labelText), FMargin(2.0f));
    }
    row->AddChildFill(valueWidget, 1.0f, FMargin(2.0f));
    return row;
}

inline std::shared_ptr<ImHorizontalBox> MakeInspectorRightAlignedPropertyRow(
    const std::string& labelText,
    const std::shared_ptr<ImWidget>& valueWidget)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);
    if (!labelText.empty()) {
        row->AddChild(MakeInspectorPropertyLabel(labelText), FMargin(2.0f));
    }
    row->AddChildFill(MakeInspectorFlexibleSpacer(), 1.0f, FMargin(2.0f));
    row->AddChild(valueWidget, FMargin(2.0f));
    return row;
}

inline std::shared_ptr<ImVerticalBox> MakeInspectorVerticalPropertyRow(
    const std::string& labelText,
    const std::shared_ptr<ImWidget>& valueWidget)
{
    auto row = std::make_shared<ImVerticalBox>();
    row->SetSpacing(4.0f);
    if (!labelText.empty()) {
        row->AddChild(MakeInspectorPropertyLabel(labelText), FMargin(2.0f, 2.0f, 2.0f, 0.0f));
    }
    row->AddChildFill(valueWidget, 1.0f, FMargin(2.0f, 0.0f, 2.0f, 2.0f));
    return row;
}

inline std::shared_ptr<ImHorizontalBox> MakeInspectorSplitValueRow(
    const std::string& labelText,
    const std::vector<std::shared_ptr<ImWidget>>& valueWidgets)
{
    auto values = std::make_shared<ImHorizontalBox>();
    values->SetSpacing(6.0f);
    for (const auto& widget : valueWidgets) {
        if (!widget) {
            continue;
        }
        values->AddChildFill(widget, 1.0f, FMargin(0.0f));
    }

    return MakeInspectorPropertyRow(labelText, values);
}

inline std::shared_ptr<ImHorizontalBox> MakeInspectorCompactLabeledEditors(
    const std::vector<std::pair<std::string, std::shared_ptr<ImWidget>>>& labeledWidgets)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(6.0f);
    for (const auto& entry : labeledWidgets) {
        auto cell = std::make_shared<ImVerticalBox>();
        cell->SetSpacing(3.0f);

        auto label = std::make_shared<ImTextBlock>();
        label->SetText(entry.first);
        label->SetWrapText(false);
        label->SetFontSize(11.0f);
        label->SetTextColor(GetEditorInspectorCompactLabelColor());

        cell->AddChild(label);
        cell->AddChildFill(entry.second, 1.0f, FMargin(0.0f));
        row->AddChildFill(cell, 1.0f, FMargin(0.0f));
    }
    return row;
}

} // namespace ImWidgetV4Editor::PropertyEditorWidgets
