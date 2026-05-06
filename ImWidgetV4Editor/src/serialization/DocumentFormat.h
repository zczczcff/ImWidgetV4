#pragma once

#include <nlohmann/json.hpp>

namespace ImWidgetV4Editor {

using json = nlohmann::ordered_json;

constexpr int kEditorDocumentFormatVersion = 1;

} // namespace ImWidgetV4Editor
