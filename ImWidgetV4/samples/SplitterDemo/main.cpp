#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
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

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImButton> MakePaneButton(
    const std::string& text,
    const FButtonStyle& style = FButtonStyle())
{
    auto button = std::make_shared<ImButton>();
    button->SetText(text);
    button->SetStyle(style);
    return button;
}

FSnapshotOptions MakeSnapshotOptions(const FFrameInfo& frameInfo)
{
    FSnapshotOptions options;
    options.Width = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.X)));
    options.Height = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.Y)));
    options.ClearColor = FColor::FromBytes(16, 20, 27);
    return options;
}

class FSplitterDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Splitter Demo - ImWidgetV4";
        config.InitialWidth = 1200;
        config.InitialHeight = 800;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"SplitterDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
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

        ExportStatus_ = std::make_shared<ImTextBlock>();
        ExportStatus_->SetText("Export the current splitter layout to a PNG snapshot.");
        ExportStatus_->SetTextColor(FColor::FromBytes(220, 228, 236));
        ExportStatus_->SetWrapText(true);
        toolbar->AddChildFill(ExportStatus_, 1.0f);

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

        application.SetRootWidget(rootLayout);

        SnapshotPath_ = std::filesystem::absolute(std::filesystem::path("artifacts") / "snapshots" / "splitter_demo.png");

        exportButton->OnClicked.AddLambda([this](ImButton&) {
            bPendingSnapshotExport_ = true;
            if (ExportStatus_) {
                ExportStatus_->SetText("Export scheduled. Snapshot will be written after this frame.");
            }
        });
    }

    void Tick(ImApplication& application, const FFrameInfo& frameInfo) override
    {
        if (!bPendingSnapshotExport_) {
            return;
        }

        bPendingSnapshotExport_ = false;

        std::error_code directoryError;
        std::filesystem::create_directories(SnapshotPath_.parent_path(), directoryError);

        FFrameContext snapshotFrameContext;
        snapshotFrameContext.FrameInfo = frameInfo;

        const bool bExported =
            !directoryError &&
            application.ExportSnapshotToPng(SnapshotPath_, snapshotFrameContext, MakeSnapshotOptions(frameInfo));

        if (!ExportStatus_) {
            return;
        }

        if (bExported) {
            ExportStatus_->SetText("Snapshot exported to: " + SnapshotPath_.string());
        } else {
            std::string message = "Snapshot export failed: " + SnapshotPath_.string();
            if (directoryError) {
                message += " (" + directoryError.message() + ")";
            }
            ExportStatus_->SetText(message);
        }
    }

private:
    std::shared_ptr<ImTextBlock> ExportStatus_;
    std::filesystem::path SnapshotPath_;
    bool bPendingSnapshotExport_ = false;
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FSplitterDemoHostDelegate>();
}

} // namespace ImWidgetV4
