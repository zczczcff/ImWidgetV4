#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4\widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImTextBlock> MakeLabel(
    const std::string& text,
    float fontSize = 16.0f,
    const FColor& color = FColor::FromBytes(230, 235, 242))
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(text);
    label->SetFontSize(fontSize);
    label->SetTextColor(color);
    label->SetWrapText(true);
    return label;
}

class ImLabeledValueCard : public ImUserWidget {
public:
    void SetLabelText(const std::string& text)
    {
        LabelText_ = text;
        if (LabelWidget_) {
            LabelWidget_->SetText(text);
        }
    }

    void SetValueText(const std::string& text)
    {
        ValueText_ = text;
        if (ValueWidget_) {
            ValueWidget_->SetText(text);
        }
    }

protected:
    Ptr RebuildWidget() override
    {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(6.0f);

        auto label = MakeLabel(LabelText_, 14.0f, FColor::FromBytes(148, 163, 184));
        label->SetName("LabelText");
        root->AddChild(label);

        auto value = MakeLabel(ValueText_, 24.0f, FColor::FromBytes(255, 214, 102));
        value->SetName("ValueText");
        root->AddChild(value);
        return root;
    }

    void OnRootWidgetRebuilt() override
    {
        LabelWidget_ = FindWidgetAs<ImTextBlock>("LabelText");
        ValueWidget_ = FindWidgetAs<ImTextBlock>("ValueText");
        if (ValueWidget_) {
            ValueWidget_->SetText(ValueText_);
        }
    }

private:
    std::string LabelText_ = "Label";
    std::string ValueText_ = "Value";
    std::shared_ptr<ImTextBlock> LabelWidget_;
    std::shared_ptr<ImTextBlock> ValueWidget_;
};

class ImCounterCard : public ImUserWidget {
public:
    void SetTitle(const std::string& title)
    {
        Title_ = title;
        if (TitleWidget_) {
            TitleWidget_->SetText(title);
        }
    }

protected:
    Ptr RebuildWidget() override
    {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(8.0f);

        auto title = MakeLabel(Title_, 15.0f, FColor::FromBytes(123, 221, 255));
        title->SetName("TitleText");
        root->AddChild(title);

        auto value = MakeLabel("0", 28.0f, FColor::FromBytes(255, 214, 102));
        value->SetName("ValueText");
        root->AddChild(value);

        auto controls = std::make_shared<ImHorizontalBox>();
        controls->SetSpacing(10.0f);

        auto incrementButton = std::make_shared<ImButton>();
        incrementButton->SetName("IncrementButton");
        incrementButton->SetText("Increment");
        controls->AddChild(incrementButton);

        auto enableCheckBox = std::make_shared<ImCheckBox>();
        enableCheckBox->SetName("EnabledCheck");
        enableCheckBox->SetLabel("Enable counting");
        enableCheckBox->SetChecked(true);
        controls->AddChild(enableCheckBox);

        root->AddChild(controls);
        return root;
    }

    void OnRootWidgetRebuilt() override
    {
        TitleWidget_ = FindWidgetAs<ImTextBlock>("TitleText");
        ValueWidget_ = FindWidgetAs<ImTextBlock>("ValueText");
        IncrementButton_ = FindWidgetAs<ImButton>("IncrementButton");
        EnabledCheckBox_ = FindWidgetAs<ImCheckBox>("EnabledCheck");

        if (TitleWidget_) {
            TitleWidget_->SetText(Title_);
        }

        if (ValueWidget_) {
            ValueWidget_->SetText(std::to_string(Value_));
        }

        if (IncrementButton_) {
            IncrementButton_->OnClicked.AddLambda([this](ImButton&) {
                if (EnabledCheckBox_ && !EnabledCheckBox_->IsChecked()) {
                    return;
                }

                ++Value_;
                if (ValueWidget_) {
                    ValueWidget_->SetText(std::to_string(Value_));
                }
            });
        }

        if (EnabledCheckBox_) {
            EnabledCheckBox_->OnCheckStateChanged.AddLambda([this](ImCheckBox&, bool checked) {
                if (IncrementButton_) {
                    IncrementButton_->SetDisabled(!checked);
                }
            });
            if (IncrementButton_) {
                IncrementButton_->SetDisabled(!EnabledCheckBox_->IsChecked());
            }
        }
    }

private:
    std::string Title_ = "Counter Card";
    int Value_ = 0;
    std::shared_ptr<ImTextBlock> TitleWidget_;
    std::shared_ptr<ImTextBlock> ValueWidget_;
    std::shared_ptr<ImButton> IncrementButton_;
    std::shared_ptr<ImCheckBox> EnabledCheckBox_;
};

class ImDashboardWidget : public ImUserWidget {
protected:
    Ptr RebuildWidget() override
    {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(14.0f);

        auto header = MakeLabel("Nested UserWidget Composition", 18.0f, FColor::FromBytes(174, 234, 119));
        root->AddChild(header);

        auto firstCounter = std::make_shared<ImCounterCard>();
        firstCounter->SetName("FirstCounter");
        firstCounter->SetTitle("Primary Counter");
        root->AddChild(firstCounter);

        auto secondCounter = std::make_shared<ImCounterCard>();
        secondCounter->SetName("SecondCounter");
        secondCounter->SetTitle("Secondary Counter");
        root->AddChild(secondCounter);

        return root;
    }
};

class FUserWidgetDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "UserWidget Demo - ImWidgetV4";
        config.InitialWidth = 980;
        config.InitialHeight = 720;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"UserWidgetDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(18.0f);

        auto title = MakeLabel("ImUserWidget Demo", 30.0f, FColor::FromBytes(255, 214, 102));
        root->AddChild(title, FMargin(20.0f, 20.0f, 16.0f, 0.0f));

        root->AddChild(
            MakeLabel(
                "ImUserWidget wraps an internal root widget tree and exposes a reusable runtime user control. These samples use RebuildWidget() to generate the tree, then cache named children with FindWidgetAs(...).",
                15.0f,
                FColor::FromBytes(214, 222, 234)),
            FMargin(20.0f, 0.0f, 20.0f, 0.0f));

        auto topRow = std::make_shared<ImHorizontalBox>();
        topRow->SetSpacing(22.0f);

        auto statusCard = std::make_shared<ImLabeledValueCard>();
        statusCard->SetLabelText("Runtime Binding");
        statusCard->SetValueText("Ready");
        topRow->AddChild(statusCard, FMargin(20.0f, 0.0f, 0.0f, 0.0f));

        auto counterCard = std::make_shared<ImCounterCard>();
        counterCard->SetTitle("Standalone Counter");
        topRow->AddChild(counterCard);

        root->AddChild(topRow);

        auto nestedDashboard = std::make_shared<ImDashboardWidget>();
        root->AddChild(nestedDashboard, FMargin(20.0f, 0.0f, 20.0f, 20.0f));

        application.SetRootWidget(root);
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FUserWidgetDemoHostDelegate>();
}

} // namespace ImWidgetV4
