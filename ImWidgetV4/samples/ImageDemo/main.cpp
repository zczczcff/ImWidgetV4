#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include "../DemoPaths.h"
#include "DemoContent.h"
#include <memory>

using namespace ImWidgetV4;

namespace {

class FImageDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Image Demo - ImWidgetV4";
        config.InitialWidth = 1080;
        config.InitialHeight = 760;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"ImageDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        application.SetApplicationIcon(application.GetCoreIconBrush(ECoreIcon::Image));
        application.SetRootWidget(Samples::CreateImageDemoRoot(application));
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FImageDemoHostDelegate>();
}

} // namespace ImWidgetV4
