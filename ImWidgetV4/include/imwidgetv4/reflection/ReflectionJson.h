#pragma once

#include <imwidgetv4/reflection/Reflectable.h>
#include <nlohmann/json_fwd.hpp>
#include <string>

namespace ImWidgetV4::Reflection {

using FReflectionJson = nlohmann::ordered_json;

struct FReflectionJsonOptions {
    EPropertyFlags ExcludedFlags = EPropertyFlags::None;
};

FReflectionJson ToJson(const IReflectable& object);
FReflectionJson ToJson(const IReflectable& object, const FReflectionJsonOptions& options);
FReflectionJson ToPersistentJson(const IReflectable& object);
FReflectionJson FilterJson(const FReflectionJson& json, const FReflectionJsonOptions& options);
FReflectionJson FilterPersistentJson(const FReflectionJson& json);
bool FromJson(IReflectable& object, const FReflectionJson& json, std::string* error = nullptr);

} // namespace ImWidgetV4::Reflection
