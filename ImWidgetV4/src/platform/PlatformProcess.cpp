#include <imwidgetv4/platform/PlatformProcess.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <sstream>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <cstdlib>
#include <sys/wait.h>
#endif

namespace ImWidgetV4 {

namespace {

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
    const FProcessOutputCallback& outputCallback)
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

std::string QuoteArgumentUtf8(const std::string& text)
{
    std::string quoted = "\"";
    for (char c : text) {
        if (c == '"') {
            quoted += "\\\"";
        } else {
            quoted.push_back(c);
        }
    }
    quoted += "\"";
    return quoted;
}

std::string JoinArgumentsForDisplay(const std::vector<std::string>& arguments)
{
    std::string commandLine;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (index > 0) {
            commandLine += " ";
        }
        commandLine += QuoteArgumentUtf8(arguments[index]);
    }
    return commandLine;
}

#if defined(_WIN32)
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

std::wstring QuoteArgumentWide(const std::wstring& text)
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

std::wstring JoinArgumentsWide(const std::vector<std::string>& arguments)
{
    std::wstring commandLine;
    for (std::size_t index = 0; index < arguments.size(); ++index) {
        if (index > 0) {
            commandLine += L" ";
        }
        commandLine += QuoteArgumentWide(Utf8ToWide(arguments[index]));
    }
    return commandLine;
}
#endif

} // namespace

std::filesystem::path GetEnvironmentPathVariable(const std::string& variableName)
{
    if (variableName.empty()) {
        return {};
    }

#if defined(_WIN32)
    const std::wstring variableNameWide = Utf8ToWide(variableName);
    wchar_t buffer[32768] = {};
    const DWORD bufferLength = static_cast<DWORD>(sizeof(buffer) / sizeof(buffer[0]));
    const DWORD length = GetEnvironmentVariableW(variableNameWide.c_str(), buffer, bufferLength);
    if (length == 0 || length >= bufferLength) {
        return {};
    }
    return std::filesystem::path(buffer).lexically_normal();
#else
    const char* value = std::getenv(variableName.c_str());
    if (value == nullptr || value[0] == '\0') {
        return {};
    }
    return std::filesystem::path(value).lexically_normal();
#endif
}

std::string BuildProcessCommandLineForDisplay(const std::vector<std::string>& arguments)
{
    return JoinArgumentsForDisplay(arguments);
}

FProcessExecutionResult ExecuteProcess(
    const std::filesystem::path& workingDirectory,
    const std::vector<std::string>& arguments,
    const FProcessOutputCallback& outputCallback)
{
    FProcessExecutionResult result;
    if (arguments.empty()) {
        result.ErrorMessage = "No process arguments were provided.";
        return result;
    }

#if defined(_WIN32)
    SECURITY_ATTRIBUTES securityAttributes {};
    securityAttributes.nLength = sizeof(securityAttributes);
    securityAttributes.bInheritHandle = TRUE;

    HANDLE readPipe = nullptr;
    HANDLE writePipe = nullptr;
    if (!CreatePipe(&readPipe, &writePipe, &securityAttributes, 0)) {
        result.ErrorMessage = "Failed to create process output pipe.";
        return result;
    }

    SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOW startupInfo {};
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.dwFlags = STARTF_USESTDHANDLES;
    startupInfo.hStdOutput = writePipe;
    startupInfo.hStdError = writePipe;

    PROCESS_INFORMATION processInformation {};
    std::wstring mutableCommandLine = JoinArgumentsWide(arguments);
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
    if (!result.bSuccess) {
        std::ostringstream error;
        error << "Process exited with code " << exitCode << ".";
        result.ErrorMessage = error.str();
    }
    return result;
#else
    std::string shellCommand;
    shellCommand += "cd ";
    shellCommand += QuoteArgumentUtf8(workingDirectory.string());
    shellCommand += " && ";
    shellCommand += JoinArgumentsForDisplay(arguments);
    shellCommand += " 2>&1";

    FILE* pipe = popen(shellCommand.c_str(), "r");
    if (pipe == nullptr) {
        result.ErrorMessage = "Failed to start process.";
        return result;
    }

    std::string pendingLine;
    char buffer[4096];
    while (fgets(buffer, static_cast<int>(sizeof(buffer)), pipe) != nullptr) {
        EmitOutputLines(buffer, pendingLine, outputCallback);
    }

    if (!pendingLine.empty()) {
        const std::string line = TrimLine(pendingLine);
        if (!line.empty() && outputCallback) {
            outputCallback(line);
        }
    }

    const int status = pclose(pipe);
    int exitCode = status;
    if (WIFEXITED(status)) {
        exitCode = WEXITSTATUS(status);
    }

    result.ExitCode = exitCode;
    result.bSuccess = exitCode == 0;
    if (!result.bSuccess) {
        std::ostringstream error;
        error << "Process exited with code " << exitCode << ".";
        result.ErrorMessage = error.str();
    }
    return result;
#endif
}

} // namespace ImWidgetV4

