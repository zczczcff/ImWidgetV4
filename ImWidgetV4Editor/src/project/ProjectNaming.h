#pragma once

#include <string>

namespace ImWidgetV4Editor {

std::string TrimWhitespaceCopy(const std::string& text);
bool EndsWithCaseInsensitive(const std::string& value, const std::string& suffix);
bool IsIdentifierStartChar(char c);
bool IsIdentifierContinueChar(char c);
bool ContainsPathSeparators(const std::string& text);
std::string NormalizeProjectIdentifier(const std::string& rawText, const std::string& fallback);
std::string NormalizeStartupDocumentFileName(const std::string& rawText);
std::string NormalizeStartupDocumentName(const std::string& rawText);
std::string BuildWidgetClassNameFromUiFileName(const std::string& uiDocumentFileName);
std::string GetDocumentDisplayTitleFromFileName(const std::string& fileName);

} // namespace ImWidgetV4Editor
