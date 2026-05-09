#pragma once

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace ImWidgetV4 {

using FProcessOutputCallback = std::function<void(const std::string&)>;

struct FProcessExecutionResult {
    bool bSuccess = false;
    int ExitCode = -1;
    std::string ErrorMessage;
};

std::filesystem::path GetEnvironmentPathVariable(const std::string& variableName);
std::string BuildProcessCommandLineForDisplay(const std::vector<std::string>& arguments);
FProcessExecutionResult ExecuteProcess(
    const std::filesystem::path& workingDirectory,
    const std::vector<std::string>& arguments,
    const FProcessOutputCallback& outputCallback);

} // namespace ImWidgetV4

