#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <memory>
#include <Windows.h>

using namespace ImWidgetV4;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    // 创建 Win32+DX11 后端
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"BoxPanel Demo - ImWidgetV4",
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

    // ==================== 创建 UI 布局 ====================

    // 创建根垂直布局
    auto rootVBox = std::make_shared<ImVerticalBox>();
    rootVBox->SetSpacing(10.0f);

    // 1. 标题文本
    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText("BoxPanel Demo - HorizontalBox & VerticalBox");
    titleText->SetTextColor(FColor::White);
    rootVBox->AddChild(titleText, FMargin(10.0f, 10.0f, 10.0f, 0.0f));

    // 2. 水平按钮组（固定大小）
    auto hBox1 = std::make_shared<ImHorizontalBox>();
    hBox1->SetSpacing(5.0f);

    auto btn1 = std::make_shared<ImButton>();
    btn1->SetText("Button 1");
    hBox1->AddChild(btn1);

    auto btn2 = std::make_shared<ImButton>();
    btn2->SetText("Button 2");
    hBox1->AddChild(btn2);

    auto btn3 = std::make_shared<ImButton>();
    btn3->SetText("Button 3");
    hBox1->AddChild(btn3);

    rootVBox->AddChild(hBox1, FMargin(10.0f, 5.0f, 10.0f, 0.0f));

    // 3. 水平按钮组（比例填充）
    auto hBox2 = std::make_shared<ImHorizontalBox>();
    hBox2->SetSpacing(5.0f);

    auto btn4 = std::make_shared<ImButton>();
    btn4->SetText("Fill 1x");
    hBox2->AddChildFill(btn4, 1.0f);

    auto btn5 = std::make_shared<ImButton>();
    btn5->SetText("Fill 2x");
    hBox2->AddChildFill(btn5, 2.0f);

    auto btn6 = std::make_shared<ImButton>();
    btn6->SetText("Fill 1x");
    hBox2->AddChildFill(btn6, 1.0f);

    rootVBox->AddChildFill(hBox2, 1.0f, FMargin(10.0f, 5.0f, 10.0f, 0.0f));

    // 4. 混合布局（固定 + 填充）
    auto hBox3 = std::make_shared<ImHorizontalBox>();
    hBox3->SetSpacing(5.0f);

    auto btn7 = std::make_shared<ImButton>();
    btn7->SetText("Fixed");
    hBox3->AddChild(btn7);

    auto btn8 = std::make_shared<ImButton>();
    btn8->SetText("Fill");
    hBox3->AddChildFill(btn8, 1.0f);

    auto btn9 = std::make_shared<ImButton>();
    btn9->SetText("Fixed");
    hBox3->AddChild(btn9);

    rootVBox->AddChildFill(hBox3, 1.0f, FMargin(10.0f, 5.0f, 10.0f, 0.0f));

    // 5. 底部文本
    auto footerText = std::make_shared<ImTextBlock>();
    footerText->SetText("Fixed size buttons vs Fill buttons");
    footerText->SetTextColor(FColor::White);
    rootVBox->AddChild(footerText, FMargin(10.0f, 0.0f, 10.0f, 10.0f));

    // 设置根控件的几何信息
    FGeometry rootGeometry;
    rootGeometry.Position = FVector2(0.0f, 0.0f);
    rootGeometry.Size = FVector2(800.0f, 600.0f);
    rootVBox->SetGeometry(rootGeometry);

    // 设置根控件
    app->SetRootWidget(rootVBox);

    // 运行主循环
    backend->Run();

    // 清理
    backend->Shutdown();

    return 0;
}
