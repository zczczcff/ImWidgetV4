#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/ReflectableObject.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace ImWidgetV4 {

class FText {
public:
    static FText FromString(std::string text);
    static FText FromKey(std::string key, std::string defaultText = std::string());

    FText() = default;

    bool IsLocalized() const { return !Key_.empty(); }
    const std::string& GetKey() const { return Key_; }
    const std::string& GetDefaultText() const { return DefaultText_; }
    const std::string& GetInvariantText() const { return InvariantText_; }

    std::string Resolve() const;
    bool operator==(const FText& other) const;
    bool operator!=(const FText& other) const { return !(*this == other); }

private:
    std::string Key_;
    std::string DefaultText_;
    std::string InvariantText_;
};

struct FStringTable {
    std::string Culture;
    std::unordered_map<std::string, std::string> Entries;
};

class FLocalizationManager {
public:
    using FCultureChangedEvent = TMulticastDelegate<const std::string&, const std::string&>;

    static FLocalizationManager& Get();

    void SetDefaultCulture(std::string culture);
    const std::string& GetDefaultCulture() const { return DefaultCulture_; }

    bool SetCulture(std::string culture);
    const std::string& GetCulture() const { return Culture_; }

    void ClearStringTables();
    bool LoadStringTableFromFile(const std::filesystem::path& path, std::string* outError = nullptr);
    bool LoadStringTableFromJson(const json& tableJson, std::string* outError = nullptr);
    void RegisterStringTable(FStringTable table);

    std::string Resolve(const FText& text) const;
    std::string Resolve(const std::string& key, const std::string& defaultText = std::string()) const;

    FCultureChangedEvent OnCultureChanged;

private:
    const FStringTable* FindTable(const std::string& culture) const;

    std::string Culture_ = "en-US";
    std::string DefaultCulture_ = "en-US";
    std::vector<FStringTable> Tables_;
};

} // namespace ImWidgetV4
