#include "BuildController.h"

#include <Windows.h>
#include <algorithm>
#include <cstdio>
#include <sstream>
#include <vector>

namespace ImWidgetV4Editor {

namespace {

std::wstring Utf8ToWide(const std::string& text)
{
    if (text.empty()) {
        return std::wstring();
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
    if (!result.empty() && result.back() == L'\0') {
        result.pop_back();
    }
    return result;
}

std::string WideToUtf8(const std::wstring& text)
{
    if (text.empty()) {
        return std::string();
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        std::string fallback;
        fallback.reserve(text.size());
        for (wchar_t c : text) {
            fallback.push_back(c >= 0 && c <= 0x7f ? static_cast<char>(c) : '?');
        }
        return fallback;
    }

    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), length, nullptr, nullptr);
    if (!result.empty() && result.back() == '\0') {
        result.pop_back();
    }
    return result;
}

std::wstring QuoteArgument(const std::wstring& text)
{
    std::wstring quoted = L"\"";
    for (wchar_t c : text) {
        if (c == L'"') {
            quoted += L"\\\"";
        } else {
            quoted.push_back(c);
        }
    }
    quoted += L"\"";
    return quoted;
}

std::wstring JoinCommandArguments(const std::vector<std::wstring>& arguments)
{
    std::wstring commandLine;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (index > 0) {
            commandLine += L" ";
        }
        commandLine += QuoteArgument(arguments[index]);
    }
    return commandLine;
}

std::string TrimLine(const std::string& text)
{
    const std::size_t first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return std::string();
    }

    const std::size_t last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1);
}

void EmitOutputLines(
    const std::string& chunk,
    std::string& pendingLine,
    const BuildController::FOutputCallback& outputCallback)
{
    if (!outputCallback || chunk.empty()) {
        return;
    }

    pendingLine += chunk;
    std::size_t newlinePosition = std::string::npos;
    while ((newlinePosition = pendingLine.find('\n')) != std::string::npos) {
        std::string line = pendingLine.substr(0, newlinePosition);
        pendingLine.erase(0, newlinePosition + 1);
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        line = TrimLine(line);
        if (!line.empty()) {
            outputCallback(line);
        }
    }
}

std::filesystem::path ResolveCompileTimeLibraryRoot()
{
    std::filesystem::path sourcePath = std::filesystem::path(__FILE__).lexically_normal();
    for (int index = 0; index < 3; ++index) {
        sourcePath = sourcePath.parent_path();
    }
    return (sourcePath.parent_path() / "ImWidgetV4").lexically_normal();
}

} // namespace

std::filesystem::path BuildController::GetDefaultBuildDirectory(
    const std::filesystem::path& projectRoot,
    const std::string& configuration)
{
    std::string normalizedConfiguration = configuration;
    std::transform(
        normalizedConfiguration.begin(),
        normalizedConfiguration.end(),
        normalizedConfiguration.begin(),
        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (normalizedConfiguration.empty()) {
        normalizedConfiguration = "debug";
    }

    return (projectRoot / "build" / ("win32-" + normalizedConfiguration)).lexically_normal();
}

std::filesystem::path BuildController::GetDefaultLibraryRoot()
{
    wchar_t buffer[32768] = {};
    const DWORD length = GetEnvironmentVariableW(L"IMWIDGETV4_ROOT", buffer, static_cast<DWORD>(std::size(buffer)));
    if (length > 0 && length < std::size(buffer)) {
        return std::filesystem::path(buffer).lexically_normal();
    }

    return ResolveCompileTimeLibraryRoot();
}

FBuildResult BuildController::ConfigureProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback,
    const std::string& configuration) const
{
    FBuildResult result;
    result.BuildDirectory = GetDefaultBuildDirectory(project.GetProjectRoot(), configuration);

    if (project.GetProjectRoot().empty()) {
        result.ErrorMessage = "Project root is empty.";
        return result;
    }

    const std::filesystem::path libraryRoot = GetDefaultLibraryRoot();
    std::vector<std::wstring> arguments = {
        L"cmake",
        L"-S", project.GetProjectRoot().wstring(),
        L"-B", result.BuildDirectory.wstring(),
        L"-DIMWIDGETV4_ROOT=" + libraryRoot.wstring()
    };

    if (outputCallback) {
        outputCallback("[configure] " + WideToUtf8(JoinCommandArguments(arguments)));
    }
    return RunProcess(project.GetProjectRoot(), JoinCommandArguments(arguments), result.BuildDirectory, outputCallback);
}

FBuildResult BuildController::BuildProject(
    const EditorProject& project,
    const FOutputCallback& outputCallback,
    const std::string& configuration) const
{
    FBuildResult configureResult;
    const std::filesystem::path buildDirectory = GetDefaultBuildDirectory(project.GetProjectRoot(), configuration);
    if (!std::filesystem::exists(buildDirectory / "CMakeCache.txt")) {
        configureResult = ConfigureProject(project, outputCallback, configuration);
        if (!configureResult.bSuccess) {
            return configureResult;
        }
    }

    std::vector<std::wstring> arguments = {
        L"cmake",
        L"--build", buildDirectory.wstring(),
        L"--config", Utf8ToWide(configuration)
    };

    if (outputCallback) {
        outputCallback("[build] " + WideToUtf8(JoinCommandArguments(arguments)));
    }
    return RunProcess(project.GetProjectRoot(), JoinCommandArguments(arguments), buildDirectory, outputCallback);
}

FBuildResult BuildController::RunProcess(
    const std::filesystem::path& workingDirectory,
    const std::wstring& commandLine,
    const std::filesystem::path& buildDirectory,
    const FOutputCallback& outputCallback)
{
    FBuildResult result;
    result.BuildDirectory = buildDirectory;

    SECURITY_ATTRIBUTES securityAttributes {};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0)) {
        result.ErrorMessage = "Failed to create build output pipe.";
        return result;
    }

    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startupInfo {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = writePipe;
    startupInfo.hStdError = writePipe;

    PROCESS_INFORMATION processInformation {};
    std::wstring mutableCommandLine = commandLine;
    const BOOL created = CreateProcessW(
        nullptr,
        mutableCommandLine.data(),
        nullptr,
        nullptr,
        TRUE,
        CREATE_NO_WINDOW,
        nullptr,
        workingDirectory.wstring().c_str(),
        &startupInfo,
        &processInformation);

    CloseHandle(writePipe);
    writePipe = nullptr;

    if (!created) {
        CloseHandle(readPipe);
        result.ErrorMessage = "Failed to start process.";
        return result;
    }

    std::string pendingLine;
    char buffer[4096];
    DWORD bytesRead = 0;
    while (ReadFile(readPipe, buffer, static_cast<DWORD>(sizeof(buffer)), &bytesRead, nullptr) && bytesRead > 0) {
        EmitOutputLines(std::string(buffer, buffer + bytesRead), pendingLine, outputCallback);
    }

    if (!pendingLine.empty()) {
        const std::string line = TrimLine(pendingLine);
        if (!line.empty() && outputCallback) {
            outputCallback(line);
        }
    }

    WaitForSingleObject(processInformation.hProcess, INFINITE);

    DWORD exitCode = 0;
    GetExitCodeProcess(processInformation.hProcess, &exitCode);
    CloseHandle(processInformation.hThread);
    CloseHandle(processInformation.hProcess);
    CloseHandle(readPipe);

    result.ExitCode = static_cast<int>(exitCode);
    result.bSuccess = exitCode == 0;
    if (!result.bSuccess && result.ErrorMessage.empty()) {
        std::ostringstream error;
        error << "Process exited with code " << exitCode << ".";
        result.ErrorMessage = error.str();
    }
    return result;
}

} // namespace ImWidgetV4Editor
