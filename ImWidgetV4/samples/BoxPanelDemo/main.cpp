#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
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

using namespace ImWidgetV4;

namespace {

FSnapshotOptions MakeSnapshotOptions(const FFrameInfo& frameInfo)
{
    FSnapshotOptions options;
    options.Width = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.X)));
    options.Height = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.Y)));
    options.ClearColor = FColor::FromBytes(18, 24, 32);
    return options;
}

class FBoxPanelDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "BoxPanel Demo - ImWidgetV4";
        config.InitialWidth = 800;
        config.InitialHeight = 600;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"BoxPanelDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        Application_ = &application;

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

        ExportStatus_ = std::make_shared<ImTextBlock>();
        ExportStatus_->SetText("Click to export the current BoxPanel demo snapshot.");
        ExportStatus_->SetTextColor(FColor::FromBytes(220, 228, 236));
        ExportStatus_->SetWrapText(true);
        toolbar->AddChildFill(ExportStatus_, 1.0f);

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

        application.SetRootWidget(rootVBox);

        SnapshotPath_ = std::filesystem::absolute(std::filesystem::path("artifacts") / "snapshots" / "box_panel_demo.png");

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
    ImApplication* Application_ = nullptr;
    std::shared_ptr<ImTextBlock> ExportStatus_;
    std::filesystem::path SnapshotPath_;
    bool bPendingSnapshotExport_ = false;
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FBoxPanelDemoHostDelegate>();
}

} // namespace ImWidgetV4
