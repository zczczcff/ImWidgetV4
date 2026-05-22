#include "ProjectSettingsSchema.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace ImWidgetV4Editor {

namespace {

bool TryParseInteger(const std::string& text, int& outValue)
{
    try {
        std::size_t processed = 0;
        const int value = std::stoi(text, &processed, 10);
        if (processed != text.size()) {
            return false;
        }

        outValue = value;
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace

std::string BoolToProjectSettingString(bool value)
{
    return value ? "true" : "false";
}

bool TryParseProjectSettingBool(const std::string& text, bool& outValue)
{
    std::string normalized = text;
    std::transform(
        normalized.begin(),
        normalized.end(),
        normalized.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on") {
        outValue = true;
        return true;
    }
    if (normalized == "0" || normalized == "false" || normalized == "no" || normalized == "off") {
        outValue = false;
        return true;
    }
    return false;
}

std::string LibraryIntegrationModeToProjectSettingString(EEditorLibraryIntegrationMode mode)
{
    return mode == EEditorLibraryIntegrationMode::SDK ? "SDK" : "Source";
}

bool TryParseLibraryIntegrationModeProjectSetting(
    const std::string& text,
    EEditorLibraryIntegrationMode& outMode)
{
    if (text == "SDK" || text == "sdk") {
        outMode = EEditorLibraryIntegrationMode::SDK;
        return true;
    }
    if (text == "Source" || text == "source") {
        outMode = EEditorLibraryIntegrationMode::Source;
        return true;
    }
    return false;
}

std::vector<std::string> GetLibraryIntegrationModeOptions()
{
    return {"Source", "SDK"};
}

int GetLibraryIntegrationModeIndex(EEditorLibraryIntegrationMode mode)
{
    return mode == EEditorLibraryIntegrationMode::SDK ? 1 : 0;
}

EEditorLibraryIntegrationMode GetLibraryIntegrationModeFromIndex(int index)
{
    return index == 1 ? EEditorLibraryIntegrationMode::SDK : EEditorLibraryIntegrationMode::Source;
}

std::string JoinProjectPathList(const std::vector<std::filesystem::path>& paths)
{
    std::string result;
    for (const std::filesystem::path& path : paths) {
        if (path.empty()) {
            continue;
        }
        if (!result.empty()) {
            result += ";";
        }
        result += path.generic_string();
    }
    return result;
}

std::vector<std::filesystem::path> ParseProjectPathList(const std::string& text)
{
    std::vector<std::filesystem::path> paths;
    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, ';')) {
        const std::size_t first = item.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;
        }
        const std::size_t last = item.find_last_not_of(" \t\r\n");
        paths.emplace_back(item.substr(first, last - first + 1));
    }
    return paths;
}

std::vector<FProjectSettingValue> GetProjectApplicationSettingValues(
    const FEditorApplicationSettings& settings,
    const std::string& key)
{
    std::vector<FProjectSettingValue> values;
    const auto appendValue = [&values, &key](const std::string& name, const std::string& value) {
        if (key.empty() || key == name) {
            values.push_back(FProjectSettingValue {name, value});
        }
    };

    appendValue("title", settings.Title);
    appendValue("libraryMode", LibraryIntegrationModeToProjectSettingString(settings.LibraryIntegrationMode));
    appendValue("sdkPath", settings.SdkPackagePath.generic_string());
    appendValue("minimumSdkVersion", settings.MinimumSdkVersion);
    appendValue("initialWidth", std::to_string(settings.InitialWidth));
    appendValue("initialHeight", std::to_string(settings.InitialHeight));
    appendValue("useTitleBar", BoolToProjectSettingString(settings.bUseTitleBar));
    appendValue("showSystemButtons", BoolToProjectSettingString(settings.bShowSystemButtons));
    appendValue("titleBarDocument", settings.TitleBarDocumentRelativePath.generic_string());
    return values;
}

FProjectSettingSetResult SetProjectApplicationSettingValue(
    FEditorApplicationSettings& settings,
    const std::string& key,
    const std::string& value)
{
    if (key == "title") {
        settings.Title = value;
    } else if (key == "libraryMode") {
        EEditorLibraryIntegrationMode mode = EEditorLibraryIntegrationMode::Source;
        if (!TryParseLibraryIntegrationModeProjectSetting(value, mode)) {
            return {false, "libraryMode must be SDK or Source."};
        }
        settings.LibraryIntegrationMode = mode;
    } else if (key == "sdkPath") {
        settings.SdkPackagePath = std::filesystem::path(value).lexically_normal();
        settings.LibraryIntegrationMode = value.empty()
            ? EEditorLibraryIntegrationMode::Source
            : EEditorLibraryIntegrationMode::SDK;
    } else if (key == "minimumSdkVersion") {
        settings.MinimumSdkVersion = value;
    } else if (key == "initialWidth") {
        int parsedValue = 0;
        if (!TryParseInteger(value, parsedValue)) {
            return {false, "initialWidth must be an integer."};
        }
        settings.InitialWidth = std::max(1, parsedValue);
    } else if (key == "initialHeight") {
        int parsedValue = 0;
        if (!TryParseInteger(value, parsedValue)) {
            return {false, "initialHeight must be an integer."};
        }
        settings.InitialHeight = std::max(1, parsedValue);
    } else if (key == "useTitleBar") {
        if (!TryParseProjectSettingBool(value, settings.bUseTitleBar)) {
            return {false, "useTitleBar must be a boolean."};
        }
    } else if (key == "showSystemButtons") {
        if (!TryParseProjectSettingBool(value, settings.bShowSystemButtons)) {
            return {false, "showSystemButtons must be a boolean."};
        }
    } else if (key == "titleBarDocument") {
        settings.TitleBarDocumentRelativePath = std::filesystem::path(value).lexically_normal();
        if (settings.TitleBarDocumentRelativePath.is_absolute()) {
            return {false, "titleBarDocument must be project-relative."};
        }
    } else {
        return {false, "Unknown project setting: " + key};
    }

    return {true, {}};
}

bool IsKnownProjectApplicationSettingKey(const std::string& key)
{
    return !GetProjectApplicationSettingValues(FEditorApplicationSettings(), key).empty();
}

} // namespace ImWidgetV4Editor
