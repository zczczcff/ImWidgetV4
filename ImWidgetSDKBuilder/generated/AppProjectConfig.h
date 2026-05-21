#pragma once

#include <imwidgetv4/app/ApplicationHost.h>

namespace ImWidgetV4 {
class ImApplication;
}

namespace GeneratedApp {

ImWidgetV4::FApplicationHostConfig BuildHostConfig();
void ConfigureApplication(ImWidgetV4::ImApplication& application);

} // namespace GeneratedApp
