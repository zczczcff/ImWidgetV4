#pragma once

#include <filesystem>

namespace ImWidgetV4Editor {

std::filesystem::path GetEditorExecutableDirectory();
std::filesystem::path GetDefaultEditorWorkspaceDirectory();
std::filesystem::path FindDefaultImWidgetV4SdkPackageDirectory();

} // namespace ImWidgetV4Editor
