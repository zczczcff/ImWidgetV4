#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
    class ImWidget;
    class ImButton;
    class ImCheckBox;
    class ImComboBox;
    class ImHorizontalBox;
    class ImTextBlock;
    class ImVerticalBox;
} // namespace ImWidgetV4

namespace ImWidgetSDKBuilder {

class MainView : public ImWidgetV4::ImUserWidget {
public:
    MainView();

protected:
    std::shared_ptr<ImWidgetV4::ImWidget> RebuildWidget() override;

private:
    //===Auto Gen Begin=== (Members)
    std::shared_ptr<ImWidgetV4::ImVerticalBox> RootLayout;
    std::shared_ptr<ImWidgetV4::ImTextBlock> TitleText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> SubtitleText;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> TopConfigRow;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> ConfigurationColumn;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildProfileHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> OutputDirectoryText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ArchitecturesLabel;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ArchitectureCheckRow;
    std::shared_ptr<ImWidgetV4::ImCheckBox> Win64CheckBox;
    std::shared_ptr<ImWidgetV4::ImCheckBox> Win32CheckBox;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ConfigurationsLabel;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ConfigurationCheckRow;
    std::shared_ptr<ImWidgetV4::ImCheckBox> DebugCheckBox;
    std::shared_ptr<ImWidgetV4::ImCheckBox> ReleaseCheckBox;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> ToolchainColumn;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ToolchainHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ToolchainLabel;
    std::shared_ptr<ImWidgetV4::ImComboBox> ToolchainComboBox;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> ActionConfigPanel;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ActionsHeader;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> StepOptionsRow;
    std::shared_ptr<ImWidgetV4::ImComboBox> BuildStepComboBox;
    std::shared_ptr<ImWidgetV4::ImComboBox> InstallerStepComboBox;
    std::shared_ptr<ImWidgetV4::ImComboBox> SmokeStepComboBox;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ActionButtonRow;
    std::shared_ptr<ImWidgetV4::ImButton> RunSelectedButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> RunSelectedButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> BuildSdkButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildSdkButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> BuildInstallerButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildInstallerButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> SmokeTestButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> SmokeTestButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> OpenPackageButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> OpenPackageButtonText;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> LogPanel;
    std::shared_ptr<ImWidgetV4::ImTextBlock> LogHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> CommandPreviewText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> LogText;
    //===Auto Gen End=== (Members)
};

} // namespace ImWidgetSDKBuilder
