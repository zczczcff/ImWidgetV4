#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/ExpandableBox.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4\widgets/TextBlock.h>
#include <imwidgetv4\widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImTextBlock> MakeBodyText(const std::string& text)
{
    auto block = std::make_shared<ImTextBlock>();
    block->SetText(text);
    block->SetWrapText(true);
    block->SetTextColor(FColor::FromBytes(222, 228, 236));
    return block;
}

std::shared_ptr<ImExpandableBox> MakeLabeledExpandable(
    const std::string& headerText,
    const std::shared_ptr<ImWidget>& body)
{
    auto header = std::make_shared<ImTextBlock>();
    header->SetText(headerText);
    header->SetTextColor(FColor::White);
    header->SetFontSize(18.0f);

    auto expandable = std::make_shared<ImExpandableBox>();
    expandable->SetHeader(header);
    expandable->SetBody(body);
    return expandable;
}

class FExpandableBoxDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "ExpandableBox Demo - ImWidgetV4";
        config.InitialWidth = 980;
        config.InitialHeight = 720;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"ExpandableBoxDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(12.0f);

        auto title = std::make_shared<ImTextBlock>();
        title->SetText("ExpandableBox Demo");
        title->SetFontSize(28.0f);
        title->SetTextColor(FColor::White);
        root->AddChild(title, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto subtitle = std::make_shared<ImTextBlock>();
        subtitle->SetText("This demo focuses on the V1 expandable panel behavior: only the triangle toggles the section, header content keeps its own interaction, and expanded body content participates in normal retained-mode routing.");
        subtitle->SetWrapText(true);
        subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
        root->AddChild(subtitle, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto status = std::make_shared<ImTextBlock>();
        status->SetText("Status: interact with any expander below.");
        status->SetTextColor(FColor::FromBytes(160, 214, 190));
        root->AddChild(status, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto basicBody = std::make_shared<ImVerticalBox>();
        basicBody->SetSpacing(10.0f);
        basicBody->AddChild(MakeBodyText("A basic expandable box can host any retained-mode subtree. This section mixes text, a checkbox, and a slider."));

        auto demoCheckBox = std::make_shared<ImCheckBox>();
        demoCheckBox->SetLabel("Enable prototype details");
        basicBody->AddChild(demoCheckBox);

        auto demoSlider = std::make_shared<ImSlider>();
        demoSlider->SetRange(0.0f, 100.0f);
        demoSlider->SetValue(42.0f);
        demoSlider->SetStep(5.0f);
        basicBody->AddChild(demoSlider);

        auto basicExpandable = MakeLabeledExpandable("Basic Expandable Section", basicBody);
        basicExpandable->SetExpanded(true);
        root->AddChild(basicExpandable, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto interactiveHeaderButton = std::make_shared<ImButton>();
        interactiveHeaderButton->SetText("Header Button (does not toggle)");

        auto headerInteractiveBody = std::make_shared<ImVerticalBox>();
        headerInteractiveBody->SetSpacing(8.0f);
        headerInteractiveBody->AddChild(MakeBodyText("The button in this header stays fully interactive. Only the triangle on the left toggles the panel."));
        auto editableText = std::make_shared<ImEditableText>();
        editableText->SetHintText("Body input remains focusable...");
        headerInteractiveBody->AddChild(editableText);

        auto headerInteractiveExpandable = std::make_shared<ImExpandableBox>();
        headerInteractiveExpandable->SetHeader(interactiveHeaderButton);
        headerInteractiveExpandable->SetBody(headerInteractiveBody);
        root->AddChild(headerInteractiveExpandable, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto nestedInnerBody = std::make_shared<ImVerticalBox>();
        nestedInnerBody->SetSpacing(6.0f);
        nestedInnerBody->AddChild(MakeBodyText("Nested expanders help with details panels, inspectors, and tree-like sections."));
        auto nestedInner = MakeLabeledExpandable("Nested Child Section", nestedInnerBody);

        auto nestedOuterBody = std::make_shared<ImVerticalBox>();
        nestedOuterBody->SetSpacing(8.0f);
        nestedOuterBody->AddChild(MakeBodyText("This outer section contains another expandable box inside its body."));
        nestedOuterBody->AddChild(nestedInner);

        auto nestedOuter = MakeLabeledExpandable("Nested Expandable Section", nestedOuterBody);
        root->AddChild(nestedOuter, FMargin(18.0f, 18.0f, 18.0f, 18.0f));

        basicExpandable->OnExpandedStateChanged.AddLambda([status](ImExpandableBox&, bool expanded) {
            status->SetText(expanded ? "Status: opened the basic section." : "Status: collapsed the basic section.");
        });

        headerInteractiveExpandable->OnExpandedStateChanged.AddLambda([status](ImExpandableBox&, bool expanded) {
            status->SetText(expanded ? "Status: opened the interactive-header section." : "Status: collapsed the interactive-header section.");
        });

        nestedOuter->OnExpandedStateChanged.AddLambda([status](ImExpandableBox&, bool expanded) {
            status->SetText(expanded ? "Status: opened the nested outer section." : "Status: collapsed the nested outer section.");
        });

        nestedInner->OnExpandedStateChanged.AddLambda([status](ImExpandableBox&, bool expanded) {
            status->SetText(expanded ? "Status: opened the nested inner section." : "Status: collapsed the nested inner section.");
        });

        interactiveHeaderButton->OnClicked.AddLambda([status](ImButton&) {
            status->SetText("Status: header button clicked without toggling its expander.");
        });

        demoCheckBox->OnCheckStateChanged.AddLambda([status](ImCheckBox&, bool checked) {
            status->SetText(checked ? "Status: checkbox enabled." : "Status: checkbox disabled.");
        });

        demoSlider->OnValueChanged.AddLambda([status](ImSlider&, float value) {
            status->SetText("Status: slider value = " + std::to_string(static_cast<int>(value)));
        });

        application.SetRootWidget(root);
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FExpandableBoxDemoHostDelegate>();
}

} // namespace ImWidgetV4
