#include "MainView.h"
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <nlohmann/json.hpp>

namespace {
    using json = nlohmann::ordered_json;

    json ParseGeneratedJson(const char* text)
    {
        return json::parse(text);
    }
} // namespace

namespace ImWidgetSDKBuilder {

MainView::MainView()
{
    SetName("MainView");
}

std::shared_ptr<ImWidgetV4::ImWidget> MainView::RebuildWidget()
{
    //===Auto Gen Begin=== (RebuildWidget)
    RootSplitter = std::make_shared<ImWidgetV4::ImVerticalSplitter>();
    RootSplitter->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalSplitter","Properties":{"ImWidget::Name":"RootSplitter","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ConfigScrollBox = std::make_shared<ImWidgetV4::ImScrollBox>();
    ConfigScrollBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImScrollBox","Properties":{"ImScrollBox::ScrollOffset":[0,0],"ImScrollBox::Style":{"Type":"FScrollBoxStyle","Properties":{"FScrollBoxStyle::CornerRadius":0,"FScrollBoxStyle::Padding":{"Type":"FMargin","Properties":{"FMargin::Left":6,"FMargin::Right":6,"FMargin::Top":6,"FMargin::Bottom":6}}}},"ImWidget::Name":"ConfigScrollBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    ConfigPanel = std::make_shared<ImWidgetV4::ImVerticalBox>();
    ConfigPanel->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalBox","Properties":{"ImVerticalBox::Spacing":8,"ImWidget::Name":"ConfigPanel","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    BuildConfigGroup = std::make_shared<ImWidgetV4::ImExpandableBox>();
    BuildConfigGroup->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImExpandableBox","Properties":{"ImExpandableBox::Expanded":true,"ImWidget::Name":"BuildConfigGroup","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true,"ImExpandableBox::Style":{"Type":"FExpandableBoxStyle","Properties":{"FExpandableBoxStyle::CornerRadius":0}}}})IMWJSON"));
    BuildConfigHeader = std::make_shared<ImWidgetV4::ImHorizontalBox>();
    BuildConfigHeader->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImHorizontalBox","Properties":{"ImHorizontalBox::Spacing":8,"ImWidget::Name":"BuildConfigHeader","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    BuildConfigIcon = std::make_shared<ImWidgetV4::ImTextBlock>();
    BuildConfigIcon->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"[B]","ImTextBlock::TextColor":[255,214,102,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Center","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"BuildConfigIcon","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    BuildConfigTitle = std::make_shared<ImWidgetV4::ImTextBlock>();
    BuildConfigTitle->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"Build","ImTextBlock::TextColor":[238,242,247,255],"ImTextBlock::FontSize":16,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"BuildConfigTitle","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    BuildConfigBody = std::make_shared<ImWidgetV4::ImVerticalBox>();
    BuildConfigBody->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalBox","Properties":{"ImVerticalBox::Spacing":10,"ImWidget::Name":"BuildConfigBody","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    OutputDirectoryText = std::make_shared<ImWidgetV4::ImTextBlock>();
    OutputDirectoryText->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"Output: build/package/ImWidgetV4-SDK","ImTextBlock::TextColor":[207,216,226,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"OutputDirectoryText","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ArchitectureCheckRow = std::make_shared<ImWidgetV4::ImHorizontalBox>();
    ArchitectureCheckRow->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImHorizontalBox","Properties":{"ImHorizontalBox::Spacing":16,"ImWidget::Name":"ArchitectureCheckRow","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    Win64CheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    Win64CheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"win64","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"Win64CheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    Win32CheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    Win32CheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"win32","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"Win32CheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    ConfigurationCheckRow = std::make_shared<ImWidgetV4::ImHorizontalBox>();
    ConfigurationCheckRow->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImHorizontalBox","Properties":{"ImHorizontalBox::Spacing":16,"ImWidget::Name":"ConfigurationCheckRow","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    DebugCheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    DebugCheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"Debug","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"DebugCheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    ReleaseCheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    ReleaseCheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"Release","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"ReleaseCheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    ToolchainConfigGroup = std::make_shared<ImWidgetV4::ImExpandableBox>();
    ToolchainConfigGroup->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImExpandableBox","Properties":{"ImExpandableBox::Expanded":true,"ImWidget::Name":"ToolchainConfigGroup","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true,"ImExpandableBox::Style":{"Type":"FExpandableBoxStyle","Properties":{"FExpandableBoxStyle::CornerRadius":0}}}})IMWJSON"));
    ToolchainConfigHeader = std::make_shared<ImWidgetV4::ImHorizontalBox>();
    ToolchainConfigHeader->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImHorizontalBox","Properties":{"ImHorizontalBox::Spacing":8,"ImWidget::Name":"ToolchainConfigHeader","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ToolchainConfigIcon = std::make_shared<ImWidgetV4::ImTextBlock>();
    ToolchainConfigIcon->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"[T]","ImTextBlock::TextColor":[123,221,255,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Center","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"ToolchainConfigIcon","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ToolchainConfigTitle = std::make_shared<ImWidgetV4::ImTextBlock>();
    ToolchainConfigTitle->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"Toolchain","ImTextBlock::TextColor":[238,242,247,255],"ImTextBlock::FontSize":16,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"ToolchainConfigTitle","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ToolchainConfigBody = std::make_shared<ImWidgetV4::ImVerticalBox>();
    ToolchainConfigBody->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalBox","Properties":{"ImVerticalBox::Spacing":8,"ImWidget::Name":"ToolchainConfigBody","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ToolchainComboBox = std::make_shared<ImWidgetV4::ImComboBox>();
    ToolchainComboBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImComboBox","Properties":{"ImComboBox::Items":["Visual Studio 17 2022","Ninja","Default"],"ImComboBox::SelectedIndex":0,"ImComboBox::PlaceholderText":"Detected toolset","ImComboBox::MaxVisibleItems":8,"ImComboBox::Disabled":false,"ImWidget::Name":"ToolchainComboBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true,"ImComboBox::Style":{"Type":"FComboBoxStyle","Properties":{"FComboBoxStyle::CornerRadius":0,"FComboBoxStyle::MinDesiredSize":[180,38]}}}})IMWJSON"));
    PackageConfigGroup = std::make_shared<ImWidgetV4::ImExpandableBox>();
    PackageConfigGroup->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImExpandableBox","Properties":{"ImExpandableBox::Expanded":true,"ImWidget::Name":"PackageConfigGroup","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true,"ImExpandableBox::Style":{"Type":"FExpandableBoxStyle","Properties":{"FExpandableBoxStyle::CornerRadius":0}}}})IMWJSON"));
    PackageConfigHeader = std::make_shared<ImWidgetV4::ImHorizontalBox>();
    PackageConfigHeader->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImHorizontalBox","Properties":{"ImHorizontalBox::Spacing":8,"ImWidget::Name":"PackageConfigHeader","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    PackageConfigIcon = std::make_shared<ImWidgetV4::ImTextBlock>();
    PackageConfigIcon->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"[P]","ImTextBlock::TextColor":[190,229,209,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Center","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"PackageConfigIcon","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    PackageConfigTitle = std::make_shared<ImWidgetV4::ImTextBlock>();
    PackageConfigTitle->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"Package","ImTextBlock::TextColor":[238,242,247,255],"ImTextBlock::FontSize":16,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"PackageConfigTitle","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    PackageConfigBody = std::make_shared<ImWidgetV4::ImVerticalBox>();
    PackageConfigBody->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalBox","Properties":{"ImVerticalBox::Spacing":10,"ImWidget::Name":"PackageConfigBody","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    TaskOptionsColumn = std::make_shared<ImWidgetV4::ImVerticalBox>();
    TaskOptionsColumn->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalBox","Properties":{"ImWidget::Name":"TaskOptionsColumn","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false,"ImVerticalBox::Spacing":8}})IMWJSON"));
    BuildSdkCheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    BuildSdkCheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"Build SDK","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"BuildSdkCheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    BuildZipCheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    BuildZipCheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"Build ZIP","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"BuildZipCheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    BuildNsisCheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    BuildNsisCheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"Build NSIS","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"BuildNsisCheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    SmokeTestCheckBox = std::make_shared<ImWidgetV4::ImCheckBox>();
    SmokeTestCheckBox->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImCheckBox","Properties":{"ImCheckBox::Label":"Smoke Test","ImCheckBox::Checked":true,"ImCheckBox::Disabled":false,"ImWidget::Name":"SmokeTestCheckBox","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    CommandPreviewText = std::make_shared<ImWidgetV4::ImTextBlock>();
    CommandPreviewText->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"> Build SDK\n  powershell\n    -NoProfile\n    -ExecutionPolicy\n    Bypass\n    -File\n    scripts/package_sdk.ps1\n    -Architectures\n    win32,win64\n    -Configurations\n    Debug,Release\n    -Generator\n    Visual Studio 17 2022\n\n> Build installer\n  powershell\n    -NoProfile\n    -ExecutionPolicy\n    Bypass\n    -File\n    scripts/package_installer.ps1\n    -Architectures\n    win32,win64\n    -Generator\n    Visual Studio 17 2022\n    -CpackGenerators\n    ZIP\n    NSIS\n    -SkipSdkBuild\n\n> Smoke test\n  powershell\n    -NoProfile\n    -ExecutionPolicy\n    Bypass\n    -File\n    scripts/smoke_sdk_package.ps1\n    -Architecture\n    win64\n    -Configuration\n    Release\n    -Generator\n    Visual Studio 17 2022","ImTextBlock::TextColor":[190,229,209,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":true,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Top","ImWidget::Name":"CommandPreviewText","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    BuildButton = std::make_shared<ImWidgetV4::ImButton>();
    BuildButton->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImButton","Properties":{"ImButton::Disabled":false,"ImWidget::Name":"BuildButton","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":true}})IMWJSON"));
    BuildButtonText = std::make_shared<ImWidgetV4::ImTextBlock>();
    BuildButtonText->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"Build","ImTextBlock::TextColor":[50,50,50,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Center","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"BuildButtonText","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    LogPanel = std::make_shared<ImWidgetV4::ImVerticalBox>();
    LogPanel->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalBox","Properties":{"ImVerticalBox::Spacing":8,"ImWidget::Name":"LogPanel","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    LogText = std::make_shared<ImWidgetV4::ImTextBlock>();
    LogText->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"Ready. Pick architectures/configurations, select the toolset, choose build steps, then run.","ImTextBlock::TextColor":[207,216,226,255],"ImTextBlock::FontSize":15,"ImTextBlock::WrapText":true,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Top","ImWidget::Name":"LogText","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));

    RootSplitter->AddChild(ConfigScrollBox);
    if (auto* slot = RootSplitter->GetSlotForChild(ConfigScrollBox)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalSplitterSlot","Properties":{"ImVerticalSplitterSlot::Ratio":2.2,"ImVerticalSplitterSlot::MinSize":320,"ImPaddingSlot::PaddingLeft":0,"ImPaddingSlot::PaddingRight":0,"ImPaddingSlot::PaddingTop":0,"ImPaddingSlot::PaddingBottom":0}})IMWJSON"));
    }
    ConfigScrollBox->SetContent(ConfigPanel);
    ConfigPanel->AddChild(BuildConfigGroup);
    if (auto* slot = ConfigPanel->GetSlotForChild(BuildConfigGroup)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0}})IMWJSON"));
    }
    BuildConfigGroup->SetHeader(BuildConfigHeader);
    BuildConfigHeader->AddChild(BuildConfigIcon);
    BuildConfigHeader->AddChild(BuildConfigTitle);
    BuildConfigGroup->SetBody(BuildConfigBody);
    BuildConfigBody->AddChild(OutputDirectoryText);
    BuildConfigBody->AddChild(ArchitectureCheckRow);
    ArchitectureCheckRow->AddChild(Win64CheckBox);
    ArchitectureCheckRow->AddChild(Win32CheckBox);
    BuildConfigBody->AddChild(ConfigurationCheckRow);
    ConfigurationCheckRow->AddChild(DebugCheckBox);
    ConfigurationCheckRow->AddChild(ReleaseCheckBox);
    ConfigPanel->AddChild(ToolchainConfigGroup);
    if (auto* slot = ConfigPanel->GetSlotForChild(ToolchainConfigGroup)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0}})IMWJSON"));
    }
    ToolchainConfigGroup->SetHeader(ToolchainConfigHeader);
    ToolchainConfigHeader->AddChild(ToolchainConfigIcon);
    ToolchainConfigHeader->AddChild(ToolchainConfigTitle);
    ToolchainConfigGroup->SetBody(ToolchainConfigBody);
    ToolchainConfigBody->AddChild(ToolchainComboBox);
    ConfigPanel->AddChild(PackageConfigGroup);
    if (auto* slot = ConfigPanel->GetSlotForChild(PackageConfigGroup)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0}})IMWJSON"));
    }
    PackageConfigGroup->SetHeader(PackageConfigHeader);
    PackageConfigHeader->AddChild(PackageConfigIcon);
    PackageConfigHeader->AddChild(PackageConfigTitle);
    PackageConfigGroup->SetBody(PackageConfigBody);
    PackageConfigBody->AddChild(TaskOptionsColumn);
    TaskOptionsColumn->AddChild(BuildSdkCheckBox);
    if (auto* slot = TaskOptionsColumn->GetSlotForChild(BuildSdkCheckBox)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0}})IMWJSON"));
    }
    TaskOptionsColumn->AddChild(BuildZipCheckBox);
    if (auto* slot = TaskOptionsColumn->GetSlotForChild(BuildZipCheckBox)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0}})IMWJSON"));
    }
    TaskOptionsColumn->AddChild(BuildNsisCheckBox);
    if (auto* slot = TaskOptionsColumn->GetSlotForChild(BuildNsisCheckBox)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0}})IMWJSON"));
    }
    TaskOptionsColumn->AddChild(SmokeTestCheckBox);
    if (auto* slot = TaskOptionsColumn->GetSlotForChild(SmokeTestCheckBox)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0,"ImPaddingSlot::PaddingLeft":0,"ImPaddingSlot::PaddingRight":0,"ImPaddingSlot::PaddingTop":0,"ImPaddingSlot::PaddingBottom":0}})IMWJSON"));
    }
    ConfigPanel->AddChild(CommandPreviewText);
    if (auto* slot = ConfigPanel->GetSlotForChild(CommandPreviewText)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0,"ImPaddingSlot::PaddingLeft":0,"ImPaddingSlot::PaddingRight":0,"ImPaddingSlot::PaddingTop":4,"ImPaddingSlot::PaddingBottom":4}})IMWJSON"));
    }
    ConfigPanel->AddChild(BuildButton);
    if (auto* slot = ConfigPanel->GetSlotForChild(BuildButton)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0,"ImPaddingSlot::PaddingLeft":0,"ImPaddingSlot::PaddingRight":0,"ImPaddingSlot::PaddingTop":0,"ImPaddingSlot::PaddingBottom":0}})IMWJSON"));
    }
    BuildButton->SetContent(BuildButtonText);
    RootSplitter->AddChild(LogPanel);
    if (auto* slot = RootSplitter->GetSlotForChild(LogPanel)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImVerticalSplitterSlot","Properties":{"ImVerticalSplitterSlot::Ratio":1,"ImVerticalSplitterSlot::MinSize":140,"ImPaddingSlot::PaddingLeft":18,"ImPaddingSlot::PaddingRight":18,"ImPaddingSlot::PaddingTop":12,"ImPaddingSlot::PaddingBottom":18}})IMWJSON"));
    }
    LogPanel->AddChild(LogText);
    if (auto* slot = LogPanel->GetSlotForChild(LogText)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":1}})IMWJSON"));
    }

    return RootSplitter;
    //===Auto Gen End=== (RebuildWidget)
}

} // namespace ImWidgetSDKBuilder
