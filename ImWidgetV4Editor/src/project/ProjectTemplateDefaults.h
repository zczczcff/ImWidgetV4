#pragma once

#include <memory>
#include <string>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

std::shared_ptr<ImWidgetV4::ImWidget> BuildDefaultStartupRoot();
std::shared_ptr<ImWidgetV4::ImWidget> BuildDefaultTitleBarRoot(const std::string& projectName);

} // namespace ImWidgetV4Editor
