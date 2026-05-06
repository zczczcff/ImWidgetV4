#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>
#include <Windows.h>

using namespace ImWidgetV4;

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"ImWidgetV4 Editor",
        1440,
        900);

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Failed to initialize editor backend.", L"ImWidgetV4 Editor", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("ImWidgetV4 Editor");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(24.0f, 24.0f, 24.0f, 0.0f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("Editor workspace is now split from the core ImWidgetV4 library. This shell target is the starting point for the editor feature set.");
    subtitle->SetWrapText(true);
    subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(subtitle, FMargin(24.0f, 0.0f, 24.0f, 24.0f));

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();
    return 0;
}
