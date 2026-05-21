// Stable user entry translation unit.
// Platform entry points are provided by the selected ImWidgetV4 app host target.

#include "AppProjectConfig.h"

#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>

#include <memory>

namespace {

class FGeneratedAppHostDelegate final : public ImWidgetV4::IApplicationHostDelegate
{
public:
    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override
    {
        ImWidgetV4::FApplicationHostConfig config = GeneratedApp::BuildHostConfig();
        return config;
    }

    void ConfigureApplication(ImWidgetV4::ImApplication& application) override
    {
        GeneratedApp::ConfigureApplication(application);
    }

    // Optional host overrides. Uncomment the functions you want to customize.
    // void ConfigureBackend(ImWidgetV4::ImApplicationBackend& backend) override
    // {
    //     (void)backend;
    // }

    // bool InitializeApplication(ImWidgetV4::ImApplication& application, ImWidgetV4::ImApplicationBackend& backend) override
    // {
    //     (void)application;
    //     (void)backend;
    //     return true;
    // }

    // void Tick(ImWidgetV4::ImApplication& application, const ImWidgetV4::FFrameInfo& frameInfo) override
    // {
    //     (void)application;
    //     (void)frameInfo;
    // }

    // bool OnCloseRequested(ImWidgetV4::ImApplication& application) override
    // {
    //     (void)application;
    //     return true;
    // }

    // void OnShutdown(ImWidgetV4::ImApplication& application) override
    // {
    //     (void)application;
    // }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FGeneratedAppHostDelegate>();
}

} // namespace ImWidgetV4
