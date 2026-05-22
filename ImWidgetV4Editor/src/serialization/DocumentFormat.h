#pragma once

#include <nlohmann/json.hpp>

namespace ImWidgetV4Editor {

using json = nlohmann::ordered_json;

constexpr int kEditorDocumentFormatVersion = 1;
constexpr const char* kEditorCodegenFieldName = "Codegen";
constexpr const char* kEditorCodegenMemberAccessFieldName = "MemberAccess";
constexpr const char* kEditorCodegenMemberAccessPublic = "Public";
constexpr const char* kEditorCodegenMemberAccessPrivate = "Private";

} // namespace ImWidgetV4Editor
