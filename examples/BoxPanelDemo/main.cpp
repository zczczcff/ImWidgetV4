#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

FSnapshotOptions MakeSnapshotOptions(const FFrameInfo& frameInfo) {
    FSnapshotOptions options;
    options.Width = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.X)));
    options.Height = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.Y)));
    options.ClearColor = FColor::FromBytes(18, 24, 32);
    return options;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"BoxPanel Demo - ImWidgetV4",
        800,
        600
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"BoxPanelDemo.ini"));
    backend->SetApplication(app.get());

    auto rootVBox = std::make_shared<ImVerticalBox>();
    rootVBox->SetSpacing(10.0f);

    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText("BoxPanel Demo - HorizontalBox & VerticalBox");
    titleText->SetTextColor(FColor::White);
    rootVBox->AddChild(titleText, FMargin(10.0f, 10.0f, 10.0f, 0.0f));

    auto toolbar = std::make_shared<ImHorizontalBox>();
    toolbar->SetSpacing(8.0f);

    auto exportButton = std::make_shared<ImButton>();
    exportButton->SetText("Export Snapshot");
    exportButton->SetStyle(FButtonStyle::CreatePrimary());
    toolbar->AddChild(exportButton);

    auto exportStatus = std::make_shared<ImTextBlock>();
    exportStatus->SetText("Click to export the current BoxPanel demo snapshot.");
    exportStatus->SetTextColor(FColor::FromBytes(220, 228, 236));
    exportStatus->SetWrapText(true);
    toolbar->AddChildFill(exportStatus, 1.0f);

    rootVBox->AddChild(toolbar, FMargin(10.0f, 5.0f, 10.0f, 0.0f));

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

    auto footerText = std::make_shared<ImTextBlock>();
    footerText->SetText("Fixed size buttons vs Fill buttons");
    footerText->SetTextColor(FColor::White);
    rootVBox->AddChild(footerText, FMargin(10.0f, 0.0f, 10.0f, 10.0f));

    FGeometry rootGeometry;
    rootGeometry.Position = FVector2(0.0f, 0.0f);
    rootGeometry.Size = FVector2(800.0f, 600.0f);
    rootVBox->SetGeometry(rootGeometry);

    app->SetRootWidget(rootVBox);

    const std::filesystem::path snapshotPath =
        std::filesystem::absolute(std::filesystem::path("artifacts") / "snapshots" / "box_panel_demo.png");
    bool bPendingSnapshotExport = false;

    exportButton->OnClicked.AddLambda([&](ImButton&) {
        bPendingSnapshotExport = true;
        exportStatus->SetText("Export scheduled. Snapshot will be written after this frame.");
    });

    backend->SetPostFrameCallback([&](const FFrameInfo& frameInfo) {
        if (!bPendingSnapshotExport) {
            return;
        }

        bPendingSnapshotExport = false;

        std::error_code directoryError;
        std::filesystem::create_directories(snapshotPath.parent_path(), directoryError);

        FFrameContext snapshotFrameContext;
        snapshotFrameContext.FrameInfo = frameInfo;

        const bool bExported =
            !directoryError &&
            app->ExportSnapshotToPng(snapshotPath, snapshotFrameContext, MakeSnapshotOptions(frameInfo));

        if (bExported) {
            exportStatus->SetText("Snapshot exported to: " + snapshotPath.string());
        } else {
            std::string message = "Snapshot export failed: " + snapshotPath.string();
            if (directoryError) {
                message += " (" + directoryError.message() + ")";
            }
            exportStatus->SetText(message);
        }
    });

    backend->Run();
    backend->Shutdown();

    return 0;
}
