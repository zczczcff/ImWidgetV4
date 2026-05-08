#include <Windows.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include "../DemoPaths.h"
#include "DemoContent.h"
#include <memory>

using namespace ImWidgetV4;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Image Demo - ImWidgetV4",
        1080,
        760
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Samples::GetDefaultSampleImGuiIniPath(L"ImageDemo.ini"));
    backend->SetApplication(app.get());
    app->SetRootWidget(Samples::CreateImageDemoRoot(*app));

    backend->Run();
    backend->Shutdown();

    return 0;
}
