#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <memory>
#include <Windows.h>

using namespace ImWidgetV4;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // 创建 Win32+DX11 后端
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Button Demo - ImWidgetV4",
        800,
        600
    );

    // 初始化后端
    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    // 创建应用程序实例
    auto app = std::make_shared<ImApplication>();

    // 设置应用程序到后端
    backend->SetApplication(app.get());

    // 创建按钮控件
    auto button = std::make_shared<ImButton>();

    // 设置文本
    button->SetText("Click Me!");

    // 设置按钮的几何信息（位置和大小）
    // 将按钮放置在窗口中央
    FGeometry buttonGeometry;
    buttonGeometry.Position = FVector2(300.0f, 285.0f);  // 居中位置 (800-200)/2, (600-30)/2
    buttonGeometry.Size = FVector2(200.0f, 30.0f);       // 按钮大小
    button->SetGeometry(buttonGeometry);

    // 设置根控件
    app->SetRootWidget(button);

    // 运行主循环
    backend->Run();

    // 清理
    backend->Shutdown();

    return 0;
}
