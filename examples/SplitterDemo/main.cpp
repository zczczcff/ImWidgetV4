#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
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

std::shared_ptr<ImButton> MakePaneButton(
    const std::string& text,
    const FButtonStyle& style = FButtonStyle()) {
    auto button = std::make_shared<ImButton>();
    button->SetText(text);
    button->SetStyle(style);
    return button;
}

FSnapshotOptions MakeSnapshotOptions(const FFrameInfo& frameInfo) {
    FSnapshotOptions options;
    options.Width = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.X)));
    options.Height = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.Y)));
    options.ClearColor = FColor::FromBytes(16, 20, 27);
    return options;
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
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"SplitterDemo.ini"));
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

    auto rootLayout = std::make_shared<ImVerticalBox>();
    rootLayout->SetSpacing(8.0f);

    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText("Splitter Demo - Horizontal & Vertical Splitters");
    titleText->SetTextColor(FColor::White);
    rootLayout->AddChild(titleText, FMargin(10.0f, 10.0f, 10.0f, 0.0f));

    auto toolbar = std::make_shared<ImHorizontalBox>();
    toolbar->SetSpacing(8.0f);

    auto exportButton = std::make_shared<ImButton>();
    exportButton->SetText("Export Snapshot");
    exportButton->SetStyle(primaryStyle);
    toolbar->AddChild(exportButton);

    auto exportStatus = std::make_shared<ImTextBlock>();
    exportStatus->SetText("Export the current splitter layout to a PNG snapshot.");
    exportStatus->SetTextColor(FColor::FromBytes(220, 228, 236));
    exportStatus->SetWrapText(true);
    toolbar->AddChildFill(exportStatus, 1.0f);

    rootLayout->AddChild(toolbar, FMargin(10.0f, 0.0f, 10.0f, 0.0f));

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

    rootLayout->AddChildFill(rootSplitter, 1.0f, FMargin(10.0f, 0.0f, 10.0f, 10.0f));

    app->SetRootWidget(rootLayout);

    const std::filesystem::path snapshotPath =
        std::filesystem::absolute(std::filesystem::path("artifacts") / "snapshots" / "splitter_demo.png");
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
