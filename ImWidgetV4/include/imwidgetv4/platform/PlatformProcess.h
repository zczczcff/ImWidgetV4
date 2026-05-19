#pragma once

#include <filesystem>
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

using FProcessOutputCallback = std::function<void(const std::string&)>;

struct FProcessExecutionResult {
    bool bSuccess = false;
    bool bCancelled = false;
    int ExitCode = -1;
    std::string ErrorMessage;
};

class FProcessCancelToken {
public:
    void RequestCancel();
    bool IsCancellationRequested() const;

private:
    std::atomic_bool bCancelRequested_ {false};
};

std::filesystem::path GetEnvironmentPathVariable(const std::string& variableName);
std::string BuildProcessCommandLineForDisplay(const std::vector<std::string>& arguments);
FProcessExecutionResult ExecuteProcess(
    const std::filesystem::path& workingDirectory,
    const std::vector<std::string>& arguments,
    const FProcessOutputCallback& outputCallback);
FProcessExecutionResult ExecuteProcess(
    const std::filesystem::path& workingDirectory,
    const std::vector<std::string>& arguments,
    const FProcessOutputCallback& outputCallback,
    const std::shared_ptr<FProcessCancelToken>& cancelToken);

} // namespace ImWidgetV4
