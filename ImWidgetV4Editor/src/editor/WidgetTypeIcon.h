#pragma once

#include <imwidgetv4/core/CoreIcon.h>

#include <optional>
#include <string>

namespace ImWidgetV4Editor {

std::optional<ImWidgetV4::ECoreIcon> TryGetWidgetTypeIcon(const std::string& typeName);

} // namespace ImWidgetV4Editor
