#pragma once

#include "EditorProject.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4Editor {

struct FProjectSettingValue {
    std::string Key;
    std::string Value;
};

struct FProjectSettingSetResult {
    bool bSuccess = false;
    std::string ErrorMessage;
};

std::string BoolToProjectSettingString(bool value);
bool TryParseProjectSettingBool(const std::string& text, bool& outValue);
std::string ColorToProjectSettingString(const ImWidgetV4::FColor& color);
bool TryParseProjectSettingColor(const std::string& text, ImWidgetV4::FColor& outColor);
std::string LibraryIntegrationModeToProjectSettingString(EEditorLibraryIntegrationMode mode);
bool TryParseLibraryIntegrationModeProjectSetting(
    const std::string& text,
    EEditorLibraryIntegrationMode& outMode);
std::string ApplicationIconSourceToProjectSettingString(EEditorApplicationIconSource source);
bool TryParseApplicationIconSourceProjectSetting(
    const std::string& text,
    EEditorApplicationIconSource& outSource);
std::vector<std::string> GetApplicationIconSourceOptions();
int GetApplicationIconSourceIndex(EEditorApplicationIconSource source);
EEditorApplicationIconSource GetApplicationIconSourceFromIndex(int index);
std::vector<std::string> GetLibraryIntegrationModeOptions();
int GetLibraryIntegrationModeIndex(EEditorLibraryIntegrationMode mode);
EEditorLibraryIntegrationMode GetLibraryIntegrationModeFromIndex(int index);
std::string JoinProjectPathList(const std::vector<std::filesystem::path>& paths);
std::vector<std::filesystem::path> ParseProjectPathList(const std::string& text);

std::vector<FProjectSettingValue> GetProjectApplicationSettingValues(
    const FEditorApplicationSettings& settings,
    const std::string& key = {});
FProjectSettingSetResult SetProjectApplicationSettingValue(
    FEditorApplicationSettings& settings,
    const std::string& key,
    const std::string& value);
bool IsKnownProjectApplicationSettingKey(const std::string& key);

} // namespace ImWidgetV4Editor
