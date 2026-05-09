#include <imwidgetv4/platform/PlatformPaths.h>

#include <system_error>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#elif defined(__linux__) || defined(__ANDROID__)
#include <limits.h>
#include <unistd.h>
#endif

namespace ImWidgetV4 {

namespace {

std::filesystem::path NormalizeOrFallback(const std::filesystem::path& path)
{
    if (path.empty()) {
        return {};
    }

    std::error_code errorCode;
    const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, errorCode);
    return errorCode ? path.lexically_normal() : canonicalPath;
}

} // namespace

std::filesystem::path GetCurrentProcessExecutableDirectory()
{
#if defined(_WIN32)
    wchar_t buffer[MAX_PATH] = {};
    const DWORD bufferLength = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    const DWORD length = ::GetModuleFileNameW(nullptr, buffer, bufferLength);
    if (length > 0 && length < bufferLength) {
        return NormalizeOrFallback(std::filesystem::path(buffer).parent_path());
    }
#elif defined(__linux__) || defined(__ANDROID__)
    char buffer[PATH_MAX] = {};
    const ssize_t length = ::readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (length > 0 && static_cast<std::size_t>(length) < sizeof(buffer)) {
        buffer[length] = '\0';
        return NormalizeOrFallback(std::filesystem::path(buffer).parent_path());
    }
#endif

    std::error_code errorCode;
    const std::filesystem::path currentPath = std::filesystem::current_path(errorCode);
    if (errorCode) {
        return {};
    }

    return NormalizeOrFallback(currentPath);
}

} // namespace ImWidgetV4
