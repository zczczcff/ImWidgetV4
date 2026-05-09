#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include "../DemoPaths.h"
#include "DemoContent.h"
#include <memory>

using namespace ImWidgetV4;

namespace {

class FInputWidgetsDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Input Widgets Demo - ImWidgetV4";
        config.InitialWidth = 960;
        config.InitialHeight = 640;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"InputWidgetsDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        application.SetRootWidget(Samples::CreateInputWidgetsDemoRoot());
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FInputWidgetsDemoHostDelegate>();
}

} // namespace ImWidgetV4
