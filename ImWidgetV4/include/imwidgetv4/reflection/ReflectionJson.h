#pragma once

#include <imwidgetv4/reflection/Reflectable.h>
#include <nlohmann/json.hpp>
#include <string>

namespace ImWidgetV4::Reflection {

using FReflectionJson = nlohmann::ordered_json;

FReflectionJson ToJson(const IReflectable& object);
bool FromJson(IReflectable& object, const FReflectionJson& json, std::string* error = nullptr);

} // namespace ImWidgetV4::Reflection
