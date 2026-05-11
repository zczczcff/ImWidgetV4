#pragma once

#include <imwidgetv4/core/Localization.h>

#include <string>

namespace ImWidgetV4Editor {

ImWidgetV4::FText EditorText(const std::string& key, const std::string& defaultText);
void RegisterEditorDefaultStringTables();

} // namespace ImWidgetV4Editor
