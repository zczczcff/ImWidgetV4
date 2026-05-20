#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/TextBlock.h>

#include <memory>

class MinimalDesktopHostDelegate final : public ImWidgetV4::IApplicationHostDelegate
{
public:
    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override
    {
        ImWidgetV4::FApplicationHostConfig config;
        config.Title = "ImWidgetV4 Minimal App";
        config.InitialWidth = 960.0f;
        config.InitialHeight = 540.0f;
        return config;
    }

    void ConfigureApplication(ImWidgetV4::ImApplication& application) override
    {
        auto text = std::make_shared<ImWidgetV4::ImTextBlock>();
        text->SetText("Hello from the ImWidgetV4 SDK");
        application.SetRootWidget(text);
    }
};

std::shared_ptr<ImWidgetV4::IApplicationHostDelegate> ImWidgetV4::CreateApplicationHostDelegate()
{
    return std::make_shared<MinimalDesktopHostDelegate>();
}
