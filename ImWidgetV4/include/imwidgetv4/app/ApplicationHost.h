#pragma once

#include <imwidgetv4/widgets/Image.h>
#include <filesystem>
#include <memory>
#include <string>

#if defined(__ANDROID__)
struct android_app;
#endif

namespace ImWidgetV4 {

class ImApplication;
class ImApplicationBackend;

struct FFrameInfo;

struct FApplicationHostConfig {
    std::string Title = "ImWidgetV4 Application";
    int InitialWidth = 1280;
    int InitialHeight = 800;
    FImageBrush Icon;
    std::filesystem::path IniSettingsPath;
    bool bUseCustomHostChrome = false;
};

class IApplicationHostDelegate {
public:
    virtual ~IApplicationHostDelegate() = default;

    virtual FApplicationHostConfig GetHostConfig() const = 0;
    virtual void ConfigureApplication(ImApplication& application) = 0;
    virtual void ConfigureBackend(ImApplicationBackend& backend);
    virtual bool InitializeApplication(ImApplication& application, ImApplicationBackend& backend);
    virtual void Tick(ImApplication& application, const FFrameInfo& frameInfo);
    virtual bool OnCloseRequested(ImApplication& application);
    virtual void OnShutdown(ImApplication& application);
};

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate();

int RunHostedDesktopApplication();

#if defined(__ANDROID__)
void RunHostedAndroidApplication(::android_app* app);
#endif

} // namespace ImWidgetV4
