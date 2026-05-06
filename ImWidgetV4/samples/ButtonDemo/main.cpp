#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include "../DemoPaths.h"
#include <memory>
#include <Windows.h>

using namespace ImWidgetV4;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // 鍒涘缓 Win32+DX11 鍚庣
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Button Demo - ImWidgetV4",
        800,
        600
    );

    // 鍒濆鍖栧悗绔
    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 鍒涘缓搴旂敤绋嬪簭瀹炰緥
    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Samples::GetDefaultSampleImGuiIniPath(L"ButtonDemo.ini"));

    // 璁剧疆搴旂敤绋嬪簭鍒板悗绔
    backend->SetApplication(app.get());

    // 鍒涘缓鎸夐挳鎺т欢
    auto button = std::make_shared<ImButton>();

    // 璁剧疆鏂囨湰
    button->SetText("Click Me!");

    // 璁剧疆鎸夐挳鐨勫嚑浣曚俊鎭紙浣嶇疆鍜屽ぇ灏忥級
    // 灏嗘寜閽斁缃湪绐楀彛涓ぎ
    FGeometry buttonGeometry;
    buttonGeometry.Position = FVector2(300.0f, 285.0f);  // 灞呬腑浣嶇疆 (800-200)/2, (600-30)/2
    buttonGeometry.Size = FVector2(200.0f, 30.0f);       // 鎸夐挳澶у皬
    button->SetGeometry(buttonGeometry);

    // 璁剧疆鏍规帶浠
    app->SetRootWidget(button);

    // 杩愯涓诲惊鐜
    backend->Run();

    // 娓呯悊
    backend->Shutdown();

    return 0;
}


