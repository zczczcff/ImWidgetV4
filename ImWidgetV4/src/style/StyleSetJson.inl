#include <imwidgetv4/style/StyleSet.h>

#include <algorithm>
#include <exception>
#include <nlohmann/json.hpp>

namespace ImWidgetV4 {

namespace {

void SetJsonError(std::string* outError, const std::string& message)
{
    if (outError != nullptr) {
        *outError = message;
    }
}

nlohmann::ordered_json ColorToJson(const FColor& color)
{
    return nlohmann::ordered_json::array({
        color.R,
        color.G,
        color.B,
        color.A
    });
}

bool TryReadColor(const nlohmann::ordered_json& value, FColor& outColor)
{
    if (value.is_array() && value.size() == 4) {
        outColor = FColor(
            value.at(0).get<float>(),
            value.at(1).get<float>(),
            value.at(2).get<float>(),
            value.at(3).get<float>());
        return true;
    }

    if (value.is_object()) {
        if (value.contains("Bytes") || value.contains("bytes")) {
            const nlohmann::ordered_json& bytes = value.contains("Bytes")
                ? value.at("Bytes")
                : value.at("bytes");
            if (bytes.is_array() && (bytes.size() == 3 || bytes.size() == 4)) {
                outColor = FColor::FromBytes(
                    bytes.at(0).get<int>(),
                    bytes.at(1).get<int>(),
                    bytes.at(2).get<int>(),
                    bytes.size() == 4 ? bytes.at(3).get<int>() : 255);
                return true;
            }
            return false;
        }

        outColor = FColor(
            value.value("R", value.value("r", 0.0f)),
            value.value("G", value.value("g", 0.0f)),
            value.value("B", value.value("b", 0.0f)),
            value.value("A", value.value("a", 1.0f)));
        return true;
    }

    return false;
}

nlohmann::ordered_json Vector2ToJson(const FVector2& value)
{
    return nlohmann::ordered_json::array({value.X, value.Y});
}

bool TryReadVector2(const nlohmann::ordered_json& value, FVector2& outValue)
{
    if (value.is_array() && value.size() == 2) {
        outValue = FVector2(value.at(0).get<float>(), value.at(1).get<float>());
        return true;
    }

    if (value.is_object()) {
        outValue = FVector2(
            value.value("X", value.value("x", 0.0f)),
            value.value("Y", value.value("y", 0.0f)));
        return true;
    }

    return false;
}

nlohmann::ordered_json MarginToJson(const FMargin& value)
{
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    json["Left"] = value.Left;
    json["Right"] = value.Right;
    json["Top"] = value.Top;
    json["Bottom"] = value.Bottom;
    return json;
}

bool TryReadMargin(const nlohmann::ordered_json& value, FMargin& outValue)
{
    if (value.is_number()) {
        outValue = FMargin(value.get<float>());
        return true;
    }

    if (value.is_array()) {
        if (value.size() == 1) {
            outValue = FMargin(value.at(0).get<float>());
            return true;
        }
        if (value.size() == 4) {
            outValue = FMargin(
                value.at(0).get<float>(),
                value.at(1).get<float>(),
                value.at(2).get<float>(),
                value.at(3).get<float>());
            return true;
        }
    }

    if (value.is_object()) {
        outValue = FMargin(
            value.value("Left", value.value("left", 0.0f)),
            value.value("Right", value.value("right", 0.0f)),
            value.value("Top", value.value("top", 0.0f)),
            value.value("Bottom", value.value("bottom", 0.0f)));
        return true;
    }

    return false;
}

template<typename TValue, typename TWriter>
nlohmann::ordered_json TokenMapToJson(
    const std::unordered_map<std::string, TValue>& values,
    TWriter writer)
{
    std::vector<std::string> keys;
    keys.reserve(values.size());
    for (const auto& entry : values) {
        keys.push_back(entry.first);
    }
    std::sort(keys.begin(), keys.end());

    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    for (const std::string& key : keys) {
        json[key] = writer(values.at(key));
    }
    return json;
}

std::shared_ptr<FStyleSet> ResolveBuiltInThemeStyleSet(const std::string& themeName)
{
    if (themeName == "Default") {
        return FStyleSetFactory::CreateDefault();
    }
    if (themeName == "Dark") {
        return FStyleSetFactory::CreateDarkTheme();
    }
    if (themeName == "Light") {
        return FStyleSetFactory::CreateLightTheme();
    }
    return nullptr;
}

} // namespace

nlohmann::ordered_json FStyleSet::ToJson() const
{
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    json["Colors"] = TokenMapToJson(Colors_, [](const FColor& value) {
        return ColorToJson(value);
    });
    json["Floats"] = TokenMapToJson(Floats_, [](float value) {
        return nlohmann::ordered_json(value);
    });
    json["Vector2"] = TokenMapToJson(Vectors_, [](const FVector2& value) {
        return Vector2ToJson(value);
    });
    json["Bools"] = TokenMapToJson(Bools_, [](bool value) {
        return nlohmann::ordered_json(value);
    });
    json["Margins"] = TokenMapToJson(Margins_, [](const FMargin& value) {
        return MarginToJson(value);
    });
    return json;
}

bool FStyleSet::FromJson(const nlohmann::ordered_json& json, std::string* outError)
{
    if (!json.is_object()) {
        SetJsonError(outError, "Style set JSON must be an object.");
        return false;
    }

    FStyleSet parsed;

    const auto parseObject = [&](const char* sectionName, const auto& parseEntry) {
        if (!json.contains(sectionName) || json.at(sectionName).is_null()) {
            return true;
        }

        const auto& section = json.at(sectionName);
        if (!section.is_object()) {
            SetJsonError(outError, std::string(sectionName) + " must be an object.");
            return false;
        }

        for (auto it = section.begin(); it != section.end(); ++it) {
            if (!parseEntry(it.key(), it.value())) {
                SetJsonError(outError, std::string("Invalid token in ") + sectionName + ": " + it.key());
                return false;
            }
        }
        return true;
    };

    if (!parseObject("Colors", [&](const std::string& key, const nlohmann::ordered_json& value) {
            FColor color;
            if (!TryReadColor(value, color)) {
                return false;
            }
            parsed.SetColor(key, color);
            return true;
        }) ||
        !parseObject("Floats", [&](const std::string& key, const nlohmann::ordered_json& value) {
            if (!value.is_number()) {
                return false;
            }
            parsed.SetFloat(key, value.get<float>());
            return true;
        }) ||
        !parseObject("Vector2", [&](const std::string& key, const nlohmann::ordered_json& value) {
            FVector2 vector;
            if (!TryReadVector2(value, vector)) {
                return false;
            }
            parsed.SetVector2(key, vector);
            return true;
        }) ||
        !parseObject("Bools", [&](const std::string& key, const nlohmann::ordered_json& value) {
            if (!value.is_boolean()) {
                return false;
            }
            parsed.SetBool(key, value.get<bool>());
            return true;
        }) ||
        !parseObject("Margins", [&](const std::string& key, const nlohmann::ordered_json& value) {
            FMargin margin;
            if (!TryReadMargin(value, margin)) {
                return false;
            }
            parsed.SetMargin(key, margin);
            return true;
        })) {
        return false;
    }

    Clear();
    Merge(parsed);
    return true;
}

FThemePack FStyleSetFactory::CreateThemePackFromJson(
    const nlohmann::ordered_json& json,
    std::string* outError)
{
    FThemePack themePack;
    if (!json.is_object()) {
        SetJsonError(outError, "Theme pack JSON must be an object.");
        return themePack;
    }

    themePack.Name = json.value("Name", std::string());
    if (themePack.Name.empty()) {
        SetJsonError(outError, "Theme pack Name is required.");
        return FThemePack();
    }

    const std::string baseThemeName = json.value("BaseTheme", std::string());
    if (!baseThemeName.empty()) {
        std::shared_ptr<FStyleSet> baseStyleSet = ResolveBuiltInThemeStyleSet(baseThemeName);
        if (!baseStyleSet) {
            SetJsonError(outError, "Unknown BaseTheme: " + baseThemeName);
            return FThemePack();
        }
        themePack.StyleSet.Merge(*baseStyleSet);
    }

    const nlohmann::ordered_json* styleSetJson = &json;
    if (json.contains("StyleSet")) {
        styleSetJson = &json.at("StyleSet");
    }

    FStyleSet overrideStyleSet;
    if (!overrideStyleSet.FromJson(*styleSetJson, outError)) {
        return FThemePack();
    }
    themePack.StyleSet.Merge(overrideStyleSet);

    return themePack;
}

FThemePack FStyleSetFactory::CreateThemePackFromJsonString(
    const std::string& jsonText,
    std::string* outError)
{
    try {
        return CreateThemePackFromJson(nlohmann::ordered_json::parse(jsonText), outError);
    } catch (const std::exception& exception) {
        SetJsonError(outError, std::string("Failed to parse theme pack JSON: ") + exception.what());
        return FThemePack();
    }
}

nlohmann::ordered_json FStyleSetFactory::ThemePackToJson(const FThemePack& themePack)
{
    nlohmann::ordered_json json = nlohmann::ordered_json::object();
    json["Name"] = themePack.Name;
    json["StyleSet"] = themePack.StyleSet.ToJson();
    return json;
}

} // namespace ImWidgetV4
