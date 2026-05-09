#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4\widgets/VerticalBox.h>
#include <memory>
#include <string>

using namespace ImWidgetV4;

namespace {

class FAndroidDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "ImWidgetV4 Android Demo";
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        application.SetApplicationIcon(application.GetCoreIconBrush(ECoreIcon::Settings));

        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(16.0f);

        auto title = std::make_shared<ImTextBlock>();
        title->SetText("ImWidgetV4 Android Backend");
        title->SetFontSize(28.0f);
        title->SetTextColor(FColor::White);
        root->AddChild(title, FMargin(20.0f, 20.0f, 20.0f, 0.0f));

        auto status = std::make_shared<ImTextBlock>();
        status->SetText("Tap the button to verify retained-mode input routing.");
        status->SetWrapText(true);
        status->SetTextColor(FColor::FromBytes(220, 227, 235));
        root->AddChild(status, FMargin(20.0f, 0.0f, 20.0f, 0.0f));

        auto button = std::make_shared<ImButton>();
        button->SetText("Tap Me");
        root->AddChild(button, FMargin(20.0f, 0.0f, 20.0f, 20.0f));

        auto tapCount = std::make_shared<int>(0);
        button->OnClicked.AddLambda([status, tapCount](ImButton&) {
            ++(*tapCount);
            status->SetText("Button tapped " + std::to_string(*tapCount) + " times.");
        });

        application.SetRootWidget(root);
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FAndroidDemoHostDelegate>();
}

} // namespace ImWidgetV4
