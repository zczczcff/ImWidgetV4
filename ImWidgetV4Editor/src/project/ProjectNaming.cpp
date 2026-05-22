#include "ProjectNaming.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace ImWidgetV4Editor {

std::string TrimWhitespaceCopy(const std::string& text)
{
    const std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }

    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix)
{
    if (suffix.size() > value.size()) {
        return false;
    }

    const std::size_t offset = value.size() - suffix.size();
    for (std::size_t index = 0; index < suffix.size(); ++index) {
        const char left = static_cast<char>(std::tolower(static_cast<unsigned char>(value[offset + index])));
        const char right = static_cast<char>(std::tolower(static_cast<unsigned char>(suffix[index])));
        if (left != right) {
            return false;
        }
    }
    return true;
}

bool IsIdentifierStartChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalpha(value) != 0 || c == '_';
}

bool IsIdentifierContinueChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalnum(value) != 0 || c == '_';
}

bool ContainsPathSeparators(const std::string& text)
{
    return text.find('/') != std::string::npos || text.find('\\') != std::string::npos;
}

std::string NormalizeProjectIdentifier(const std::string& rawText, const std::string& fallback)
{
    const std::string source = TrimWhitespaceCopy(rawText);
    const std::string fallbackSource = TrimWhitespaceCopy(fallback).empty()
        ? std::string("AppProject")
        : TrimWhitespaceCopy(fallback);

    auto sanitize = [](const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (char c : text) {
            if (IsIdentifierContinueChar(c)) {
                result.push_back(c);
            } else if (result.empty() || result.back() != '_') {
                result.push_back('_');
            }
        }

        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }
        return result;
    };

    std::string normalized = sanitize(source);
    if (normalized.empty()) {
        normalized = sanitize(fallbackSource);
    }
    if (normalized.empty()) {
        normalized = "AppProject";
    }
    if (!IsIdentifierStartChar(normalized.front())) {
        normalized.insert(normalized.begin(), '_');
    }
    return normalized;
}

std::string NormalizeStartupDocumentFileName(const std::string& rawText)
{
    std::string trimmedName = TrimWhitespaceCopy(rawText);
    if (trimmedName.empty()) {
        trimmedName = "MainView";
    }
    if (EndsWithCaseInsensitive(trimmedName, ".ui.json")) {
        return trimmedName;
    }

    const std::filesystem::path path(trimmedName);
    const std::string stem = path.stem().string();
    const std::string baseName = stem.empty() ? trimmedName : stem;
    return baseName + ".ui.json";
}

std::string NormalizeStartupDocumentName(const std::string& rawText)
{
    const std::string trimmed = TrimWhitespaceCopy(rawText);
    if (trimmed.empty()) {
        return "MainView";
    }

    return std::filesystem::path(trimmed).stem().string();
}

std::string BuildWidgetClassNameFromUiFileName(const std::string& uiDocumentFileName)
{
    std::string baseName = uiDocumentFileName;
    if (EndsWithCaseInsensitive(baseName, ".ui.json")) {
        baseName.resize(baseName.size() - std::string(".ui.json").size());
    } else {
        baseName = std::filesystem::path(baseName).stem().string();
    }

    std::string className = NormalizeProjectIdentifier(baseName, "GeneratedWidget");
    if (!className.empty()) {
        className.front() = static_cast<char>(std::toupper(static_cast<unsigned char>(className.front())));
    }
    return className;
}

std::string GetDocumentDisplayTitleFromFileName(const std::string& fileName)
{
    std::string title = fileName;
    if (EndsWithCaseInsensitive(title, ".ui.json")) {
        title.resize(title.size() - std::string(".ui.json").size());
    } else {
        title = std::filesystem::path(title).stem().string();
    }

    return title.empty() ? std::string("Main") : title;
}

} // namespace ImWidgetV4Editor
