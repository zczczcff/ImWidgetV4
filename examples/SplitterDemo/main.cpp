#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <memory>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImButton> MakePaneButton(
    const std::string& text,
    const FButtonStyle& style = FButtonStyle()) {
    auto button = std::make_shared<ImButton>();
    button->SetText(text);
    button->SetStyle(style);
    return button;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Splitter Demo - ImWidgetV4",
        1200,
        800
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    backend->SetApplication(app.get());

    FHorizontalSplitterStyle horizontalStyle;
    horizontalStyle.BarWidth = 8.0f;
    horizontalStyle.Color = FColor::FromBytes(44, 54, 67);
    horizontalStyle.HoveredColor = FColor::FromBytes(77, 97, 122);
    horizontalStyle.ActiveColor = FColor::FromBytes(231, 111, 81);
    horizontalStyle.Rounding = 3.0f;

    FVerticalSplitterStyle verticalStyle;
    verticalStyle.BarHeight = 8.0f;
    verticalStyle.Color = FColor::FromBytes(44, 54, 67);
    verticalStyle.HoveredColor = FColor::FromBytes(77, 97, 122);
    verticalStyle.ActiveColor = FColor::FromBytes(42, 157, 143);
    verticalStyle.Rounding = 3.0f;

    auto primaryStyle = FButtonStyle::CreatePrimary();
    auto dangerStyle = FButtonStyle::CreateDanger();

    auto rootSplitter = std::make_shared<ImVerticalSplitter>();
    rootSplitter->SetSplitterStyle(verticalStyle);

    auto topHorizontal = std::make_shared<ImHorizontalSplitter>();
    topHorizontal->SetSplitterStyle(horizontalStyle);

    auto nestedVertical = std::make_shared<ImVerticalSplitter>();
    nestedVertical->SetSplitterStyle(verticalStyle);
    nestedVertical->AddPart(MakePaneButton("Inspector", primaryStyle), 1.0f, 120.0f, FMargin(8.0f));
    nestedVertical->AddPart(MakePaneButton("Console"), 1.0f, 120.0f, FMargin(8.0f));
    nestedVertical->AddPart(MakePaneButton("Details", dangerStyle), 1.0f, 120.0f, FMargin(8.0f));

    topHorizontal->AddPart(MakePaneButton("Project", primaryStyle), 1.0f, 180.0f, FMargin(10.0f));
    topHorizontal->AddPart(nestedVertical, 1.4f, 240.0f, FMargin(10.0f));
    topHorizontal->AddPart(MakePaneButton("Preview"), 1.2f, 220.0f, FMargin(10.0f));

    auto bottomVertical = std::make_shared<ImVerticalSplitter>();
    bottomVertical->SetSplitterStyle(verticalStyle);
    bottomVertical->AddPart(MakePaneButton("Timeline"), 1.0f, 90.0f, FMargin(8.0f, 8.0f, 8.0f, 4.0f));
    bottomVertical->AddPart(MakePaneButton("Output", dangerStyle), 1.2f, 90.0f, FMargin(8.0f, 8.0f, 4.0f, 4.0f));
    bottomVertical->AddPart(MakePaneButton("Properties", primaryStyle), 0.8f, 90.0f, FMargin(8.0f, 8.0f, 4.0f, 8.0f));

    rootSplitter->AddPart(topHorizontal, 2.0f, 280.0f);
    rootSplitter->AddPart(bottomVertical, 1.2f, 220.0f);

    app->SetRootWidget(rootSplitter);
    backend->Run();
    backend->Shutdown();

    return 0;
}
