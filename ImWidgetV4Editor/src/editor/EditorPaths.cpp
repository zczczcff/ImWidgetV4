#include "EditorPaths.h"
#include <imwidgetv4/platform/PlatformPaths.h>

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
    const std::filesystem::path executableDirectory = ImWidgetV4::GetCurrentProcessExecutableDirectory();
    if (executableDirectory.empty()) {
        return NormalizeOrFallback(std::filesystem::current_path());
    }

    return NormalizeOrFallback(executableDirectory);
}

std::filesystem::path GetDefaultEditorWorkspaceDirectory()
{
    const std::filesystem::path workspaceDirectory = GetEditorExecutableDirectory() / "DefaultWorkspace";
    std::error_code error;
    std::filesystem::create_directories(workspaceDirectory, error);
    return NormalizeOrFallback(workspaceDirectory);
}

} // namespace ImWidgetV4Editor
