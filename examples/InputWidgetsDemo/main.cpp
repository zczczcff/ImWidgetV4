#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <cstdio>
#include <memory>
#include <string>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

std::string FormatSliderValue(float value) {
    char buffer[64] = {};
    std::snprintf(buffer, sizeof(buffer), "Slider Value: %.2f", value);
    return buffer;
}

std::string FormatComboSelection(const ImComboBox& comboBox) {
    if (!comboBox.HasSelection()) {
        return "Combo Selection: (none)";
    }

    return "Combo Selection: " + comboBox.GetSelectedText();
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Input Widgets Demo - ImWidgetV4",
        960,
        640
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"InputWidgetsDemo.ini"));
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("Input Widgets Demo");
    title->SetFontSize(26.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(16.0f, 16.0f, 16.0f, 0.0f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("CheckBox controls enabled state, ComboBox opens a popup list, EditableText reports live and committed text, and Slider supports click, drag, and keyboard adjustment.");
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
    root->AddChild(slider, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto sliderValue = std::make_shared<ImTextBlock>();
    sliderValue->SetText(FormatSliderValue(slider->GetValue()));
    sliderValue->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(sliderValue, FMargin(16.0f, 0.0f, 16.0f, 0.0f));

    auto actionsRow = std::make_shared<ImHorizontalBox>();
    actionsRow->SetSpacing(10.0f);

    auto resetButton = std::make_shared<ImButton>();
    resetButton->SetText("Reset Demo");
    resetButton->SetStyle(FButtonStyle::CreatePrimary());
    actionsRow->AddChild(resetButton);

    auto fillSampleButton = std::make_shared<ImButton>();
    fillSampleButton->SetText("Load Sample Text");
    actionsRow->AddChild(fillSampleButton);

    root->AddChild(actionsRow, FMargin(16.0f, 12.0f, 16.0f, 16.0f));

    disableControls->OnCheckStateChanged.AddLambda([&](ImCheckBox&, bool checked) {
        comboBox->SetDisabled(checked);
        editableText->SetDisabled(checked);
        slider->SetDisabled(checked);
        liveText->SetText(checked ? "Live Text: controls are disabled" : "Live Text: " + editableText->GetText());
    });

    comboBox->OnSelectionChanged.AddLambda([&](ImComboBox& sender, int) {
        comboSelection->SetText(FormatComboSelection(sender));
    });

    editableText->OnTextChanged.AddLambda([&](ImEditableText&, const std::string& text) {
        liveText->SetText("Live Text: " + text);
    });

    editableText->OnTextCommitted.AddLambda([&](ImEditableText&, const std::string& text) {
        committedText->SetText("Committed Text: " + text);
    });

    slider->OnValueChanged.AddLambda([&](ImSlider&, float value) {
        sliderValue->SetText(FormatSliderValue(value));
    });

    resetButton->OnClicked.AddLambda([&](ImButton&) {
        disableControls->SetChecked(false);
        comboBox->SetDisabled(false);
        editableText->SetDisabled(false);
        slider->SetDisabled(false);
        comboBox->SetSelectedIndex(2);
        editableText->SetText("ImWidgetV4");
        slider->SetValue(42.0f);
        comboSelection->SetText(FormatComboSelection(*comboBox));
        liveText->SetText("Live Text: ImWidgetV4");
        committedText->SetText("Committed Text: (not committed yet)");
        sliderValue->SetText(FormatSliderValue(slider->GetValue()));
    });

    fillSampleButton->OnClicked.AddLambda([&](ImButton&) {
        comboBox->SetSelectedIndex(4);
        editableText->SetText("ComboBox + EditableText + Slider");
        comboSelection->SetText(FormatComboSelection(*comboBox));
        committedText->SetText("Committed Text: sample loaded");
    });

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();

    return 0;
}
