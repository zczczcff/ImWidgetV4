#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace ImWidgetV4 {

enum class EPathDialogResultCode : unsigned char {
    Accepted,
    Cancelled,
    Unsupported,
    Error
};

struct FPathDialogResult {
    EPathDialogResultCode Code = EPathDialogResultCode::Unsupported;
    std::filesystem::path Path;
    std::string ErrorMessage;

    bool IsAccepted() const
    {
        return Code == EPathDialogResultCode::Accepted;
    }
};

struct FFileDialogFilter {
    std::string Label;
    std::vector<std::string> Patterns;
};

struct FOpenFileDialogOptions {
    std::string Title;
    std::filesystem::path InitialDirectory;
    std::vector<FFileDialogFilter> Filters;
    int DefaultFilterIndex = 0;
};

struct FOpenFolderDialogOptions {
    std::string Title;
    std::filesystem::path InitialDirectory;
};

struct FSaveFileDialogOptions {
    std::string Title;
    std::filesystem::path InitialDirectory;
    std::string DefaultFileName;
    std::string DefaultExtension;
    std::vector<FFileDialogFilter> Filters;
    int DefaultFilterIndex = 0;
    bool bPromptOverwrite = true;
};

} // namespace ImWidgetV4
