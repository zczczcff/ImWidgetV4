#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include "../DemoPaths.h"
#include <memory>

using namespace ImWidgetV4;

namespace {

class FButtonDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Button Demo - ImWidgetV4";
        config.InitialWidth = 800;
        config.InitialHeight = 600;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"ButtonDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        auto button = std::make_shared<ImButton>();
        button->SetText("Click Me!");

        FGeometry buttonGeometry;
        buttonGeometry.Position = FVector2(300.0f, 285.0f);
        buttonGeometry.Size = FVector2(200.0f, 30.0f);
        button->SetGeometry(buttonGeometry);

        application.SetRootWidget(button);
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FButtonDemoHostDelegate>();
}

} // namespace ImWidgetV4
