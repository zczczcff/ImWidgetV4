#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
    class ImWidget;
    class ImButton;
    class ImCheckBox;
    class ImComboBox;
    class ImExpandableBox;
    class ImHorizontalBox;
    class ImScrollBox;
    class ImTextBlock;
    class ImVerticalBox;
    class ImVerticalSplitter;
} // namespace ImWidgetV4

namespace ImWidgetSDKBuilder {

class MainView : public ImWidgetV4::ImUserWidget {
public:
    MainView();

protected:
    std::shared_ptr<ImWidgetV4::ImWidget> RebuildWidget() override;

private:
    //===Auto Gen Begin=== (Members)
    std::shared_ptr<ImWidgetV4::ImVerticalSplitter> RootSplitter;
    std::shared_ptr<ImWidgetV4::ImScrollBox> ConfigScrollBox;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> ConfigPanel;
    std::shared_ptr<ImWidgetV4::ImExpandableBox> BuildConfigGroup;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> BuildConfigHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildConfigIcon;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildConfigTitle;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> BuildConfigBody;
    std::shared_ptr<ImWidgetV4::ImTextBlock> OutputDirectoryText;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ArchitectureCheckRow;
    std::shared_ptr<ImWidgetV4::ImCheckBox> Win64CheckBox;
    std::shared_ptr<ImWidgetV4::ImCheckBox> Win32CheckBox;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ConfigurationCheckRow;
    std::shared_ptr<ImWidgetV4::ImCheckBox> DebugCheckBox;
    std::shared_ptr<ImWidgetV4::ImCheckBox> ReleaseCheckBox;
    std::shared_ptr<ImWidgetV4::ImExpandableBox> ToolchainConfigGroup;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ToolchainConfigHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ToolchainConfigIcon;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ToolchainConfigTitle;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> ToolchainConfigBody;
    std::shared_ptr<ImWidgetV4::ImComboBox> ToolchainComboBox;
    std::shared_ptr<ImWidgetV4::ImExpandableBox> PackageConfigGroup;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> PackageConfigHeader;
    std::shared_ptr<ImWidgetV4::ImTextBlock> PackageConfigIcon;
    std::shared_ptr<ImWidgetV4::ImTextBlock> PackageConfigTitle;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> PackageConfigBody;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> StepOptionsRow;
    std::shared_ptr<ImWidgetV4::ImComboBox> BuildStepComboBox;
    std::shared_ptr<ImWidgetV4::ImComboBox> InstallerStepComboBox;
    std::shared_ptr<ImWidgetV4::ImComboBox> SmokeStepComboBox;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> ActionButtonColumn;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ActionButtonRow;
    std::shared_ptr<ImWidgetV4::ImButton> RunSelectedButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> RunSelectedButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> BuildSdkButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildSdkButtonText;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> ActionButtonRow2;
    std::shared_ptr<ImWidgetV4::ImButton> BuildInstallerButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> BuildInstallerButtonText;
    std::shared_ptr<ImWidgetV4::ImButton> SmokeTestButton;
    std::shared_ptr<ImWidgetV4::ImTextBlock> SmokeTestButtonText;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> LogPanel;
    std::shared_ptr<ImWidgetV4::ImTextBlock> CommandPreviewText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> LogText;
    //===Auto Gen End=== (Members)
};

} // namespace ImWidgetSDKBuilder
