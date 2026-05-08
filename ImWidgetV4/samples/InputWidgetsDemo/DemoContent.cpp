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
#include <functional>
#include <memory>
#include <string>

namespace ImWidgetV4::Samples {

namespace {

class FInputTraceVerticalBox : public ImVerticalBox {
public:
    void SetInputTraceCallback(std::function<void(const FInputEvent&)> callback)
    {
        Callback_ = std::move(callback);
    }

    FReply OnPreviewInputEvent(const FInputEvent& event) override
    {
        if (Callback_ != nullptr &&
            (event.Type == EInputEventType::KeyDown ||
             event.Type == EInputEventType::KeyUp ||
             event.Type == EInputEventType::TextInput)) {
            Callback_(event);
        }

        return ImVerticalBox::OnPreviewInputEvent(event);
    }

private:
    std::function<void(const FInputEvent&)> Callback_;
};

class FTraceEditableText : public ImEditableText {
public:
    void SetTraceCallback(std::function<void(const FInputEvent&)> callback)
    {
        Callback_ = std::move(callback);
    }

    FReply OnInputEvent(const FInputEvent& event) override
    {
        if (Callback_ != nullptr &&
            (event.Type == EInputEventType::KeyDown ||
             event.Type == EInputEventType::KeyUp ||
             event.Type == EInputEventType::TextInput)) {
            Callback_(event);
        }

        return ImEditableText::OnInputEvent(event);
    }

private:
    std::function<void(const FInputEvent&)> Callback_;
};

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

std::string FormatInputModifiers(const FInputModifiers& modifiers)
{
    std::string result;
    if (modifiers.bCtrl) {
        result += "Ctrl+";
    }
    if (modifiers.bShift) {
        result += "Shift+";
    }
    if (modifiers.bAlt) {
        result += "Alt+";
    }
    if (modifiers.bSuper) {
        result += "Super+";
    }

    if (result.empty()) {
        return "(none)";
    }

    result.pop_back();
    return result;
}

const char* GetKeyName(EKey key)
{
    switch (key) {
    case EKey::Enter: return "Enter";
    case EKey::Space: return "Space";
    case EKey::Tab: return "Tab";
    case EKey::Escape: return "Escape";
    case EKey::Backspace: return "Backspace";
    case EKey::DeleteKey: return "Delete";
    case EKey::Left: return "Left";
    case EKey::Right: return "Right";
    case EKey::Up: return "Up";
    case EKey::Down: return "Down";
    case EKey::Home: return "Home";
    case EKey::End: return "End";
    case EKey::PageUp: return "PageUp";
    case EKey::PageDown: return "PageDown";
    case EKey::A: return "A";
    case EKey::B: return "B";
    case EKey::C: return "C";
    case EKey::D: return "D";
    case EKey::E: return "E";
    case EKey::F: return "F";
    case EKey::G: return "G";
    case EKey::H: return "H";
    case EKey::I: return "I";
    case EKey::J: return "J";
    case EKey::K: return "K";
    case EKey::L: return "L";
    case EKey::M: return "M";
    case EKey::N: return "N";
    case EKey::O: return "O";
    case EKey::P: return "P";
    case EKey::Q: return "Q";
    case EKey::R: return "R";
    case EKey::S: return "S";
    case EKey::T: return "T";
    case EKey::U: return "U";
    case EKey::V: return "V";
    case EKey::W: return "W";
    case EKey::X: return "X";
    case EKey::Y: return "Y";
    case EKey::Z: return "Z";
    case EKey::Num0: return "0";
    case EKey::Num1: return "1";
    case EKey::Num2: return "2";
    case EKey::Num3: return "3";
    case EKey::Num4: return "4";
    case EKey::Num5: return "5";
    case EKey::Num6: return "6";
    case EKey::Num7: return "7";
    case EKey::Num8: return "8";
    case EKey::Num9: return "9";
    case EKey::F1: return "F1";
    case EKey::F2: return "F2";
    case EKey::F3: return "F3";
    case EKey::F4: return "F4";
    case EKey::F5: return "F5";
    case EKey::F6: return "F6";
    case EKey::F7: return "F7";
    case EKey::F8: return "F8";
    case EKey::F9: return "F9";
    case EKey::F10: return "F10";
    case EKey::F11: return "F11";
    case EKey::F12: return "F12";
    default: return "Unknown";
    }
}

std::string FormatCodepoint(unsigned int codepoint)
{
    char buffer[64] = {};
    if (codepoint >= 32U && codepoint <= 126U) {
        std::snprintf(
            buffer,
            sizeof(buffer),
            "'%c' (U+%04X)",
            static_cast<char>(codepoint),
            codepoint);
    } else {
        std::snprintf(buffer, sizeof(buffer), "U+%04X", codepoint);
    }

    return buffer;
}

} // namespace

std::shared_ptr<ImWidget> CreateInputWidgetsDemoRoot()
{
    auto root = std::make_shared<FInputTraceVerticalBox>();
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

    auto editableText = std::make_shared<FTraceEditableText>();
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

    auto traceLabel = std::make_shared<ImTextBlock>();
    traceLabel->SetText("Input Trace");
    traceLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(traceLabel, FMargin(16.0f, 8.0f, 16.0f, 0.0f));

    auto traceHint = std::make_shared<ImTextBlock>();
    traceHint->SetText("Use soft keyboard, hardware keyboard, or adb keyevent injection to verify Android key bridging.");
    traceHint->SetWrapText(true);
    traceHint->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(traceHint, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto traceSequence = std::make_shared<ImTextBlock>();
    traceSequence->SetText("Last Event #: 0");
    traceSequence->SetTextColor(FColor::White);
    root->AddChild(traceSequence, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto traceEvent = std::make_shared<ImTextBlock>();
    traceEvent->SetText("Last Event: (none)");
    traceEvent->SetWrapText(true);
    traceEvent->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(traceEvent, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto traceModifiers = std::make_shared<ImTextBlock>();
    traceModifiers->SetText("Modifiers: (none)");
    traceModifiers->SetTextColor(FColor::FromBytes(160, 214, 190));
    root->AddChild(traceModifiers, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

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

    auto traceCounter = std::make_shared<int>(0);
    auto updateTrace = [=](const FInputEvent& event) {
        ++(*traceCounter);

        char sequenceBuffer[64] = {};
        std::snprintf(sequenceBuffer, sizeof(sequenceBuffer), "Last Event #: %d", *traceCounter);
        traceSequence->SetText(sequenceBuffer);

        if (event.Type == EInputEventType::TextInput) {
            traceEvent->SetText("Last Event: TextInput " + FormatCodepoint(event.Codepoint));
        } else {
            const char* eventName =
                event.Type == EInputEventType::KeyDown ? "KeyDown" :
                event.Type == EInputEventType::KeyUp ? "KeyUp" : "Other";
            traceEvent->SetText(
                "Last Event: " +
                std::string(eventName) +
                " " +
                GetKeyName(event.Key));
        }

        traceModifiers->SetText("Modifiers: " + FormatInputModifiers(event.Modifiers));
    };

    root->SetInputTraceCallback(updateTrace);
    editableText->SetTraceCallback(updateTrace);

    return root;
}

} // namespace ImWidgetV4::Samples
