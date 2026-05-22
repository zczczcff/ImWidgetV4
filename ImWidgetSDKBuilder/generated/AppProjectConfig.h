#pragma once

#include <imwidgetv4/app/ApplicationHost.h>

#include <memory>

namespace ImWidgetSDKBuilder {
class MainView;
class TitleBarView;
} // namespace ImWidgetSDKBuilder

namespace ImWidgetV4 {
class ImApplication;
}

namespace GeneratedApp {

ImWidgetV4::FApplicationHostConfig BuildHostConfig();
void ConfigureApplication(ImWidgetV4::ImApplication& application);
std::shared_ptr<ImWidgetSDKBuilder::MainView> GetStartupView();
std::shared_ptr<ImWidgetSDKBuilder::TitleBarView> GetTitleBarView();

} // namespace GeneratedApp
