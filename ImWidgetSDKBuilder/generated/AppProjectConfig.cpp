#include "AppProjectConfig.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <filesystem>
#include <memory>
#include <string>

#include "MainView.h"

namespace GeneratedApp {

ImWidgetV4::FApplicationHostConfig BuildHostConfig()
{
        ImWidgetV4::FApplicationHostConfig config;
        config.Title = "ImWidgetSDKBuilder";
        config.InitialWidth = 1280;
        config.InitialHeight = 720;
        config.bUseCustomHostChrome = true;
        return config;
}

void ConfigureApplication(ImWidgetV4::ImApplication& application)
{
        application.SetApplicationTitle("ImWidgetSDKBuilder");
        application.SetRootWidget(std::make_shared<ImWidgetSDKBuilder::MainView>());
}

} // namespace GeneratedApp
