#include "DemoContent.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/Switch.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <cstdio>
#include <memory>
#include <string>

namespace ImWidgetV4::Samples {

namespace {

std::string FormatSliderValue(float value)
{
    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "Slider Value: %.2f", value);
    return buffer;
}

std::string FormatComboSelection(const ImComboBox& comboBox)
{
    if (!comboBox.HasSelection()) {
        return "Combo Selection: (none)";
    }

    return "Combo Selection: " + comboBox.GetSelectedText();
}

std::string FormatSwitchState(bool checked)
{
    return checked ? "Switch State: On" : "Switch State: Off";
}

} // namespace

std::shared_ptr<ImWidget> CreateInputWidgetsDemoRoot()
{
    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("Input Widgets Demo");
    title->SetFontSize(26.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(16.0f, 16.0f, 16.0f, 0.0f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("CheckBox controls enabled state, ComboBox opens a popup list, EditableText reports live and committed text, Slider supports click/drag/keyboard adjustment, and Switch provides a compact on/off toggle.");
    subtitle->SetWrapText(true);
    subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(subtitle, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto disableControls = std::make_shared<ImCheckBox>();
    disableControls->SetLabel("Disable combo box, text box, and slider");
    root->AddChild(disableControls, FMargin(16.0f, 4.0f, 16.0f, 0.0f));

    auto comboLabel = std::make_shared<ImTextBlock>();
    comboLabel->SetText("ComboBox");
    comboLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(comboLabel, FMargin(16.0f, 8.0f, 16.0f, 0.0f));

    auto comboBox = std::make_shared<ImComboBox>();
    comboBox->SetPlaceholderText("Choose a workflow preset...");
    comboBox->SetItems({
        "Prototype UI",
        "Polish Theme",
        "Write Tests",
        "Capture Snapshot",
        "Ship Demo"
    });
    comboBox->SetSelectedIndex(2);
    comboBox->SetToolTipText("Hover to preview the system tooltip, then click to open the retained-mode popup list.");
    root->AddChild(comboBox, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto comboSelection = std::make_shared<ImTextBlock>();
    comboSelection->SetText(FormatComboSelection(*comboBox));
    comboSelection->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(comboSelection, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto editableLabel = std::make_shared<ImTextBlock>();
    editableLabel->SetText("Single-line EditableText");
    editableLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(editableLabel, FMargin(16.0f, 8.0f, 16.0f, 0.0f));

    auto editableText = std::make_shared<ImEditableText>();
    editableText->SetHintText("Type here, press Enter to commit...");
    editableText->SetText("ImWidgetV4");
    editableText->SetToolTipText("EditableText supports live change callbacks and committed text updates.");
    root->AddChild(editableText, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto liveText = std::make_shared<ImTextBlock>();
    liveText->SetText("Live Text: ImWidgetV4");
    liveText->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(liveText, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto committedText = std::make_shared<ImTextBlock>();
    committedText->SetText("Committed Text: (not committed yet)");
    committedText->SetTextColor(FColor::FromBytes(160, 214, 190));
    root->AddChild(committedText, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto sliderLabel = std::make_shared<ImTextBlock>();
    sliderLabel->SetText("Horizontal Slider");
    sliderLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(sliderLabel, FMargin(16.0f, 8.0f, 16.0f, 0.0f));

    auto slider = std::make_shared<ImSlider>();
    slider->SetRange(0.0f, 100.0f);
    slider->SetValue(42.0f);
    slider->SetStep(5.0f);
    slider->SetToolTipText("Drag or use keyboard nudges after focusing the slider.");
    root->AddChild(slider, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto sliderValue = std::make_shared<ImTextBlock>();
    sliderValue->SetText(FormatSliderValue(slider->GetValue()));
    sliderValue->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(sliderValue, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto switchLabel = std::make_shared<ImTextBlock>();
    switchLabel->SetText("Switch");
    switchLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(switchLabel, FMargin(16.0f, 8.0f, 16.0f, 0.0f));

    auto switchRow = std::make_shared<ImHorizontalBox>();
    switchRow->SetSpacing(12.0f);

    auto switchWidget = std::make_shared<ImSwitch>();
    switchWidget->SetToolTipText("Switch is the compact boolean toggle control.");
    switchRow->AddChild(switchWidget);

    auto switchHint = std::make_shared<ImTextBlock>();
    switchHint->SetText("Space / Enter can toggle after focusing the switch.");
    switchHint->SetTextColor(FColor::FromBytes(214, 222, 234));
    switchRow->AddChild(switchHint);

    root->AddChild(switchRow, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto switchState = std::make_shared<ImTextBlock>();
    switchState->SetText(FormatSwitchState(switchWidget->IsChecked()));
    switchState->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(switchState, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto disabledSwitchRow = std::make_shared<ImHorizontalBox>();
    disabledSwitchRow->SetSpacing(12.0f);

    auto disabledSwitch = std::make_shared<ImSwitch>();
    disabledSwitch->SetChecked(true);
    disabledSwitch->SetDisabled(true);
    disabledSwitchRow->AddChild(disabledSwitch);

    auto disabledSwitchHint = std::make_shared<ImTextBlock>();
    disabledSwitchHint->SetText("Disabled switch style example");
    disabledSwitchHint->SetTextColor(FColor::FromBytes(160, 214, 190));
    disabledSwitchRow->AddChild(disabledSwitchHint);

    root->AddChild(disabledSwitchRow, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto actionsRow = std::make_shared<ImHorizontalBox>();
    actionsRow->SetSpacing(10.0f);

    auto resetButton = std::make_shared<ImButton>();
    resetButton->SetText("Reset Demo");
    resetButton->SetStyle(FButtonStyle::CreatePrimary());
    resetButton->SetToolTipText("Restore the default sample values for every input widget.");
    actionsRow->AddChild(resetButton);

    auto fillSampleButton = std::make_shared<ImButton>();
    fillSampleButton->SetText("Load Sample Text");
    fillSampleButton->SetToolTipText("Populate the inputs with a different preset so state changes are easy to inspect.");
    actionsRow->AddChild(fillSampleButton);

    root->AddChild(actionsRow, FMargin(16.0f, 12.0f, 16.0f, 16.0f));

    disableControls->OnCheckStateChanged.AddLambda([=](ImCheckBox&, bool checked) {
        comboBox->SetDisabled(checked);
        editableText->SetDisabled(checked);
        slider->SetDisabled(checked);
        switchWidget->SetDisabled(checked);
        liveText->SetText(checked ? "Live Text: controls are disabled" : "Live Text: " + editableText->GetText());
    });

    comboBox->OnSelectionChanged.AddLambda([=](ImComboBox& sender, int) {
        comboSelection->SetText(FormatComboSelection(sender));
    });

    editableText->OnTextChanged.AddLambda([=](ImEditableText&, const std::string& text) {
        liveText->SetText("Live Text: " + text);
    });

    editableText->OnTextCommitted.AddLambda([=](ImEditableText&, const std::string& text) {
        committedText->SetText("Committed Text: " + text);
    });

    slider->OnValueChanged.AddLambda([=](ImSlider&, float value) {
        sliderValue->SetText(FormatSliderValue(value));
    });

    switchWidget->OnCheckStateChanged.AddLambda([=](ImSwitch&, bool checked) {
        switchState->SetText(FormatSwitchState(checked));
    });

    resetButton->OnClicked.AddLambda([=](ImButton&) {
        disableControls->SetChecked(false);
        comboBox->SetDisabled(false);
        editableText->SetDisabled(false);
        slider->SetDisabled(false);
        switchWidget->SetDisabled(false);
        comboBox->SetSelectedIndex(2);
        editableText->SetText("ImWidgetV4");
        slider->SetValue(42.0f);
        switchWidget->SetChecked(false);
        comboSelection->SetText(FormatComboSelection(*comboBox));
        liveText->SetText("Live Text: ImWidgetV4");
        committedText->SetText("Committed Text: (not committed yet)");
        sliderValue->SetText(FormatSliderValue(slider->GetValue()));
        switchState->SetText(FormatSwitchState(switchWidget->IsChecked()));
    });

    fillSampleButton->OnClicked.AddLambda([=](ImButton&) {
        comboBox->SetSelectedIndex(4);
        editableText->SetText("ComboBox + EditableText + Slider");
        comboSelection->SetText(FormatComboSelection(*comboBox));
        committedText->SetText("Committed Text: sample loaded");
    });

    return root;
}

} // namespace ImWidgetV4::Samples
