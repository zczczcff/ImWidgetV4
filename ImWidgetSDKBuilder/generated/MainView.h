#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
    class ImWidget;
    class ImButton;
    class ImCanvasPanel;
    class ImTextBlock;
} // namespace ImWidgetV4

namespace ImWidgetSDKBuilder {

class MainView : public ImWidgetV4::ImUserWidget {
public:
    MainView();

protected:
    std::shared_ptr<ImWidgetV4::ImWidget> RebuildWidget() override;

private:
    //===Auto Gen Begin=== (Members)
    std::shared_ptr<ImWidgetV4::ImCanvasPanel> RootCanvas;
    std::shared_ptr<ImWidgetV4::ImTextBlock> TitleText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> SubtitleText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildProfileHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> OutputDirectoryText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ArchitecturesLabel;
    std::shared_ptr<ImWidgetV4::ImButton> Win64Button;
    std::shared_ptr<ImWidgetV4::ImTextBlock> Win64ButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> Win32Button;
    std::shared_ptr<ImWidgetV4::ImTextBlock> Win32ButtonText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ConfigurationsLabel;
    std::shared_ptr<ImWidgetV4::ImButton> DebugButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> DebugButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> ReleaseButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ReleaseButtonText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> PackageOutputsLabel;
    std::shared_ptr<ImWidgetV4::ImButton> StagingButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> StagingButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> ZipButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ZipButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> NsisButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> NsisButtonText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ToolchainHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ToolchainSummaryText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ActionsHeader;
    std::shared_ptr<ImWidgetV4::ImButton> BuildSdkButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildSdkButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> BuildInstallerButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildInstallerButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> SmokeTestButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> SmokeTestButtonText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> CommandPreviewHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> CommandPreviewText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> LogHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> LogText;
    //===Auto Gen End=== (Members)
};

} // namespace ImWidgetSDKBuilder
