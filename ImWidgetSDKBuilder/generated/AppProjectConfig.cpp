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
#include "TitleBarView.h"

namespace GeneratedApp {

ImWidgetV4::FApplicationHostConfig BuildHostConfig()
{
        ImWidgetV4::FApplicationHostConfig config;
        config.Title = "ImWidget SDK Builder";
        config.InitialWidth = 640;
        config.InitialHeight = 720;
        config.bUseCustomHostChrome = true;
        return config;
}

void ConfigureApplication(ImWidgetV4::ImApplication& application)
{
        application.SetApplicationTitle("ImWidget SDK Builder");
        auto rootLayout = std::make_shared<ImWidgetV4::ImVerticalBox>();
        rootLayout->SetSpacing(0.0f);
        auto titleBarView = std::make_shared<ImWidgetSDKBuilder::TitleBarView>();
        rootLayout->AddChild(titleBarView, ImWidgetV4::FMargin(0.0f));
        rootLayout->AddChildFill(std::make_shared<ImWidgetSDKBuilder::MainView>(), 1.0f, ImWidgetV4::FMargin(0.0f));
        application.SetRootWidget(rootLayout);
}

} // namespace GeneratedApp
