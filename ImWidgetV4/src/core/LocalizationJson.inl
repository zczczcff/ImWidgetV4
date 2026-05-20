#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Application.h>

#include <fstream>
#include <nlohmann/json.hpp>

namespace ImWidgetV4 {

bool ImApplication::LoadStringTable(const std::filesystem::path& path, std::string* outError)
{
    return FLocalizationManager::Get().LoadStringTableFromFile(path, outError);
}

bool FLocalizationManager::LoadStringTableFromFile(const std::filesystem::path& path, std::string* outError)
{
    try {
        std::ifstream stream(path, std::ios::binary);
        if (!stream.is_open()) {
            if (outError) {
                *outError = "Failed to open localization file: " + path.string();
            }
            return false;
        }

        json tableJson;
        stream >> tableJson;
        return LoadStringTableFromJson(tableJson, outError);
    } catch (const std::exception& e) {
        if (outError) {
            *outError = e.what();
        }
        return false;
    }
}

bool FLocalizationManager::LoadStringTableFromJson(const json& tableJson, std::string* outError)
{
    if (!tableJson.is_object()) {
        if (outError) {
            *outError = "Localization table must be a JSON object.";
        }
        return false;
    }

    FStringTable table;
    table.Culture = tableJson.value("Culture", std::string());
    if (table.Culture.empty()) {
        if (outError) {
            *outError = "Localization table is missing Culture.";
        }
        return false;
    }

    if (!tableJson.contains("Entries") || !tableJson.at("Entries").is_object()) {
        if (outError) {
            *outError = "Localization table is missing Entries object.";
        }
        return false;
    }

    for (auto it = tableJson.at("Entries").begin(); it != tableJson.at("Entries").end(); ++it) {
        if (it.value().is_string()) {
            table.Entries[it.key()] = it.value().get<std::string>();
        }
    }

    RegisterStringTable(std::move(table));
    return true;
}

} // namespace ImWidgetV4
