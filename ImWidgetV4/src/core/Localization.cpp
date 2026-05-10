#include <imwidgetv4/core/Localization.h>

#include <fstream>
#include <utility>

namespace ImWidgetV4 {

FText FText::FromString(std::string text)
{
    FText result;
    result.InvariantText_ = std::move(text);
    return result;
}

FText FText::FromKey(std::string key, std::string defaultText)
{
    FText result;
    result.Key_ = std::move(key);
    result.DefaultText_ = std::move(defaultText);
    return result;
}

std::string FText::Resolve() const
{
    return FLocalizationManager::Get().Resolve(*this);
}

bool FText::operator==(const FText& other) const
{
    return Key_ == other.Key_ &&
        DefaultText_ == other.DefaultText_ &&
        InvariantText_ == other.InvariantText_;
}

FLocalizationManager& FLocalizationManager::Get()
{
    static FLocalizationManager manager;
    return manager;
}

void FLocalizationManager::SetDefaultCulture(std::string culture)
{
    if (culture.empty()) {
        culture = "en-US";
    }

    DefaultCulture_ = std::move(culture);
}

bool FLocalizationManager::SetCulture(std::string culture)
{
    if (culture.empty()) {
        return false;
    }

    if (Culture_ == culture) {
        return true;
    }

    const std::string oldCulture = Culture_;
    Culture_ = std::move(culture);
    OnCultureChanged.Broadcast(oldCulture, Culture_);
    return true;
}

void FLocalizationManager::ClearStringTables()
{
    Tables_.clear();
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

void FLocalizationManager::RegisterStringTable(FStringTable table)
{
    if (table.Culture.empty()) {
        return;
    }

    for (FStringTable& existingTable : Tables_) {
        if (existingTable.Culture == table.Culture) {
            existingTable.Entries = std::move(table.Entries);
            return;
        }
    }

    Tables_.push_back(std::move(table));
}

std::string FLocalizationManager::Resolve(const FText& text) const
{
    if (!text.IsLocalized()) {
        return text.GetInvariantText();
    }

    return Resolve(text.GetKey(), text.GetDefaultText());
}

std::string FLocalizationManager::Resolve(const std::string& key, const std::string& defaultText) const
{
    if (key.empty()) {
        return defaultText;
    }

    if (const FStringTable* table = FindTable(Culture_)) {
        const auto it = table->Entries.find(key);
        if (it != table->Entries.end()) {
            return it->second;
        }
    }

    if (DefaultCulture_ != Culture_) {
        if (const FStringTable* table = FindTable(DefaultCulture_)) {
            const auto it = table->Entries.find(key);
            if (it != table->Entries.end()) {
                return it->second;
            }
        }
    }

    return defaultText.empty() ? key : defaultText;
}

const FStringTable* FLocalizationManager::FindTable(const std::string& culture) const
{
    for (const FStringTable& table : Tables_) {
        if (table.Culture == culture) {
            return &table;
        }
    }

    return nullptr;
}

} // namespace ImWidgetV4
