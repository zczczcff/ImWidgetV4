#include "EditorPaths.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

namespace ImWidgetV4Editor {

namespace {

std::filesystem::path NormalizeOrFallback(const std::filesystem::path& path)
{
    if (path.empty()) {
        return {};
    }

    std::error_code error;
    const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, error);
    return error ? path.lexically_normal() : canonicalPath;
}

} // namespace

std::filesystem::path GetEditorExecutableDirectory()
{
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = ::GetModuleFileNameW(nullptr, buffer, static_cast<DWORD>(std::size(buffer)));
    if (length == 0 || length >= std::size(buffer)) {
        return NormalizeOrFallback(std::filesystem::current_path());
    }

    return NormalizeOrFallback(std::filesystem::path(buffer).parent_path());
}

std::filesystem::path GetDefaultEditorWorkspaceDirectory()
{
    const std::filesystem::path workspaceDirectory = GetEditorExecutableDirectory() / "DefaultWorkspace";
    std::error_code error;
    std::filesystem::create_directories(workspaceDirectory, error);
    return NormalizeOrFallback(workspaceDirectory);
}

} // namespace ImWidgetV4Editor
