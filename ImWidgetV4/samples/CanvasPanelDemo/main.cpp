#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>
#include <Windows.h>

using namespace ImWidgetV4;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"CanvasPanel Demo - ImWidgetV4",
        960,
        640);

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Samples::GetDefaultSampleImGuiIniPath(L"CanvasPanelDemo.ini"));
    backend->SetApplication(app.get());

    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetDesiredSize(FVector2(960.0f, 640.0f));

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("CanvasPanel Demo");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    canvas->AddChildAt(title, FVector2(0.04f, 0.05f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("Relative-positioned children resize with the canvas, while auto-size widgets keep their own desired size.");
    subtitle->SetWrapText(true);
    subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
    canvas->AddChildAt(subtitle, FVector2(0.04f, 0.12f), FVector2(0.56f, 0.10f));

    auto primaryButton = std::make_shared<ImButton>();
    primaryButton->SetText("Relative Size Button");
    primaryButton->SetStyle(FButtonStyle::CreatePrimary());
    canvas->AddChildAt(primaryButton, FVector2(0.06f, 0.28f), FVector2(0.28f, 0.10f));

    auto autoCheckBox = std::make_shared<ImCheckBox>();
    autoCheckBox->SetLabel("Auto-sized CheckBox");
    canvas->AddChildAt(autoCheckBox, FVector2(0.62f, 0.24f));

    auto autoSlider = std::make_shared<ImSlider>();
    autoSlider->SetRange(0.0f, 100.0f);
    autoSlider->SetValue(42.0f);
    autoSlider->SetStep(5.0f);
    canvas->AddChildAt(autoSlider, FVector2(0.62f, 0.36f));

    auto overlapBack = std::make_shared<ImButton>();
    overlapBack->SetText("Back Layer");
    canvas->AddChildAt(overlapBack, FVector2(0.52f, 0.62f), FVector2(0.22f, 0.11f));

    auto overlapFront = std::make_shared<ImButton>();
    overlapFront->SetText("Front Layer");
    overlapFront->SetStyle(FButtonStyle::CreatePrimary());
    canvas->AddChildAt(overlapFront, FVector2(0.58f, 0.67f), FVector2(0.24f, 0.11f));

    auto status = std::make_shared<ImTextBlock>();
    status->SetText("Click the overlapping buttons to confirm front-most hit testing.");
    status->SetTextColor(FColor::FromBytes(255, 214, 102));
    canvas->AddChildAt(status, FVector2(0.04f, 0.88f), FVector2(0.58f, 0.07f));

    primaryButton->OnClicked.AddLambda([&](ImButton&) {
        status->SetText("Relative-size button clicked.");
    });

    overlapBack->OnClicked.AddLambda([&](ImButton&) {
        status->SetText("Back-layer button clicked.");
    });

    overlapFront->OnClicked.AddLambda([&](ImButton&) {
        status->SetText("Front-layer button clicked.");
    });

    autoCheckBox->OnCheckStateChanged.AddLambda([&](ImCheckBox&, bool checked) {
        status->SetText(checked ? "Auto-sized CheckBox enabled." : "Auto-sized CheckBox disabled.");
    });

    autoSlider->OnValueChanged.AddLambda([&](ImSlider&, float value) {
        status->SetText("Auto-sized Slider value: " + std::to_string(static_cast<int>(value)));
    });

    app->SetRootWidget(canvas);
    backend->Run();
    backend->Shutdown();

    return 0;
}


