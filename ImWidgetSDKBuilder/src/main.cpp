// Stable user entry translation unit.
// Platform entry points are provided by the selected ImWidgetV4 app host target.

#include "AppProjectConfig.h"
#include "MainView.h"
#include "TitleBarView.h"

#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/CoreIcon.h>
#include <imwidgetv4/platform/PlatformProcess.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

std::vector<std::string> DetectToolsetOptions()
{
    std::vector<std::string> options;
    std::set<std::string> seen;

    auto addOption = [&options, &seen](const std::string& option) {
        if (!option.empty() && seen.insert(option).second) {
            options.push_back(option);
        }
    };

#if defined(_WIN32)
    const std::filesystem::path programFilesX86 = ImWidgetV4::GetEnvironmentPathVariable("ProgramFiles(x86)");
    if (!programFilesX86.empty()) {
        const std::filesystem::path vswherePath =
            programFilesX86 / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
        if (std::filesystem::exists(vswherePath)) {
            std::vector<std::string> lines;
            const ImWidgetV4::FProcessExecutionResult result = ImWidgetV4::ExecuteProcess(
                std::filesystem::current_path(),
                {
                    vswherePath.string(),
                    "-products",
                    "*",
                    "-requires",
                    "Microsoft.Component.MSBuild",
                    "-property",
                    "catalog_productLineVersion"
                },
                [&lines](const std::string& line) {
                    if (!line.empty()) {
                        lines.push_back(line);
                    }
                });
            if (result.bSuccess) {
                for (const std::string& line : lines) {
                    if (line == "2022") {
                        addOption("Visual Studio 17 2022");
                    } else if (line == "2019") {
                        addOption("Visual Studio 16 2019");
                    } else if (line == "2017") {
                        addOption("Visual Studio 15 2017");
                    }
                }
            }
        }
    }
#endif

    const ImWidgetV4::FProcessExecutionResult ninjaResult = ImWidgetV4::ExecuteProcess(
        std::filesystem::current_path(),
        {"ninja", "--version"},
        nullptr);
    if (ninjaResult.bSuccess) {
        addOption("Ninja");
    }

    if (options.empty()) {
        addOption("Default");
    }

    return options;
}

bool IsCommandAvailable(const std::string& command)
{
    return ImWidgetV4::ExecuteProcess(
        std::filesystem::current_path(),
        {"where", command},
        nullptr).bSuccess;
}

template<typename T>
std::shared_ptr<T> FindWidgetInTree(const std::shared_ptr<ImWidgetV4::ImWidget>& widget)
{
    if (!widget) {
        return nullptr;
    }

    if (auto typedWidget = std::dynamic_pointer_cast<T>(widget)) {
        return typedWidget;
    }

    for (const std::shared_ptr<ImWidgetV4::ImWidget>& child : widget->GetChildren()) {
        if (auto typedWidget = FindWidgetInTree<T>(child)) {
            return typedWidget;
        }
    }

    return nullptr;
}

void BindAtLeastOneChecked(
    const std::shared_ptr<ImWidgetV4::ImCheckBox>& first,
    const std::shared_ptr<ImWidgetV4::ImCheckBox>& second)
{
    if (!first || !second) {
        return;
    }

    auto enforceFirst = [first, second](ImWidgetV4::ImCheckBox&, bool checked) {
        if (!checked && second && !second->IsChecked()) {
            first->SetChecked(true);
        }
    };
    auto enforceSecond = [first, second](ImWidgetV4::ImCheckBox&, bool checked) {
        if (!checked && first && !first->IsChecked()) {
            second->SetChecked(true);
        }
    };

    first->OnCheckStateChanged.AddLambda(enforceFirst);
    second->OnCheckStateChanged.AddLambda(enforceSecond);
}

std::vector<std::string> CollectCheckedArchitectures(
    const std::shared_ptr<ImWidgetV4::ImCheckBox>& win64,
    const std::shared_ptr<ImWidgetV4::ImCheckBox>& win32)
{
    std::vector<std::string> architectures;
    if (win32 && win32->IsChecked()) {
        architectures.push_back("win32");
    }
    if (win64 && win64->IsChecked()) {
        architectures.push_back("win64");
    }
    return architectures;
}

std::vector<std::string> CollectCheckedConfigurations(
    const std::shared_ptr<ImWidgetV4::ImCheckBox>& debug,
    const std::shared_ptr<ImWidgetV4::ImCheckBox>& release)
{
    std::vector<std::string> configurations;
    if (debug && debug->IsChecked()) {
        configurations.push_back("Debug");
    }
    if (release && release->IsChecked()) {
        configurations.push_back("Release");
    }
    return configurations;
}

std::string JoinValues(const std::vector<std::string>& values, const std::string& separator)
{
    std::ostringstream stream;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            stream << separator;
        }
        stream << values[index];
    }
    return stream.str();
}

void SetTextIfValid(const std::shared_ptr<ImWidgetV4::ImTextBlock>& textBlock, const std::string& text)
{
    if (textBlock) {
        textBlock->SetText(text);
    }
}

void RunBuilderCommand(
    const std::string& label,
    const std::vector<std::string>& arguments,
    std::ostringstream& logStream,
    bool& bAllSucceeded)
{
    logStream << "> " << label << "\n";
    logStream << ImWidgetV4::BuildProcessCommandLineForDisplay(arguments) << "\n";

    const ImWidgetV4::FProcessExecutionResult result = ImWidgetV4::ExecuteProcess(
        std::filesystem::current_path(),
        arguments,
        [&logStream](const std::string& line) {
            logStream << line << "\n";
        });

    if (!result.bSuccess) {
        bAllSucceeded = false;
        logStream << "Failed";
        if (result.ExitCode >= 0) {
            logStream << " with exit code " << result.ExitCode;
        }
        if (!result.ErrorMessage.empty()) {
            logStream << ": " << result.ErrorMessage;
        }
        logStream << "\n";
        return;
    }

    logStream << "Done.\n";
}

void AppendGeneratorArguments(std::vector<std::string>& arguments, const std::string& generator)
{
    if (generator.empty() || generator == "Default") {
        return;
    }

    arguments.push_back("-Generator");
    arguments.push_back(generator);
}

void ConfigureBuilderUi(ImWidgetV4::ImApplication& application)
{
    auto titleBarView = FindWidgetInTree<ImWidgetSDKBuilder::TitleBarView>(application.GetRootWidget());
    if (titleBarView) {
        auto titleBarIcon = titleBarView->FindWidgetAs<ImWidgetV4::ImImage>("TitleBarIcon");
        if (titleBarIcon) {
            titleBarIcon->SetBrush(application.GetApplicationIcon());
        }
    }

    auto mainView = FindWidgetInTree<ImWidgetSDKBuilder::MainView>(application.GetRootWidget());
    if (!mainView) {
        return;
    }

    auto win64 = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("Win64CheckBox");
    auto win32 = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("Win32CheckBox");
    auto debug = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("DebugCheckBox");
    auto release = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("ReleaseCheckBox");
    auto buildSdk = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("BuildSdkCheckBox");
    auto buildZip = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("BuildZipCheckBox");
    auto buildNsis = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("BuildNsisCheckBox");
    auto smokeTest = mainView->FindWidgetAs<ImWidgetV4::ImCheckBox>("SmokeTestCheckBox");
    auto buildButton = mainView->FindWidgetAs<ImWidgetV4::ImButton>("BuildButton");
    auto commandPreview = mainView->FindWidgetAs<ImWidgetV4::ImTextBlock>("CommandPreviewText");
    auto logText = mainView->FindWidgetAs<ImWidgetV4::ImTextBlock>("LogText");
    BindAtLeastOneChecked(win64, win32);
    BindAtLeastOneChecked(debug, release);

    auto toolchain = mainView->FindWidgetAs<ImWidgetV4::ImComboBox>("ToolchainComboBox");
    if (toolchain) {
        const std::vector<std::string> options = DetectToolsetOptions();
        toolchain->SetItems(options);
        toolchain->SetSelectedIndex(options.empty() ? -1 : 0);
    }

    const bool bNsisAvailable = IsCommandAvailable("makensis");
    if (buildNsis) {
        buildNsis->SetDisabled(!bNsisAvailable);
        buildNsis->SetChecked(bNsisAvailable && buildNsis->IsChecked());
    }
    if (!bNsisAvailable) {
        SetTextIfValid(logText, "Ready. NSIS was not found on PATH, so Build NSIS is disabled.");
    }

    SetTextIfValid(commandPreview, "Select tasks, then click Build.");

    if (buildButton) {
        buildButton->OnClicked.AddLambda([=](ImWidgetV4::ImButton& button) {
            std::ostringstream logStream;
            bool bAllSucceeded = true;

            const std::vector<std::string> architectures = CollectCheckedArchitectures(win64, win32);
            const std::vector<std::string> configurations = CollectCheckedConfigurations(debug, release);
            const std::string architectureList = JoinValues(architectures, ",");
            const std::string configurationList = JoinValues(configurations, ",");
            const bool bBuildSdk = buildSdk && buildSdk->IsChecked();
            const bool bBuildZip = buildZip && buildZip->IsChecked();
            const bool bBuildNsis = buildNsis && buildNsis->IsChecked() && !buildNsis->IsDisabled();
            const bool bSmokeTest = smokeTest && smokeTest->IsChecked();
            const std::string selectedGenerator = toolchain ? toolchain->GetSelectedText() : std::string();

            if (!bBuildSdk && !bBuildZip && !bBuildNsis && !bSmokeTest) {
                SetTextIfValid(logText, "No build tasks selected.");
                return;
            }

            SetTextIfValid(commandPreview, "Build in progress...");
            button.SetDisabled(true);

            if (bBuildSdk) {
                std::vector<std::string> sdkArgs {
                    "powershell",
                    "-NoProfile",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    "scripts/package_sdk.ps1",
                    "-Architectures",
                    architectureList,
                    "-Configurations",
                    configurationList
                };
                AppendGeneratorArguments(sdkArgs, selectedGenerator);
                RunBuilderCommand("Build SDK", sdkArgs, logStream, bAllSucceeded);
            }

            if (bBuildZip || bBuildNsis) {
                std::vector<std::string> installerArgs {
                    "powershell",
                    "-NoProfile",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    "scripts/package_installer.ps1",
                    "-Architectures",
                    architectureList
                };
                AppendGeneratorArguments(installerArgs, selectedGenerator);
                installerArgs.push_back("-CpackGenerators");
                if (bBuildZip) {
                    installerArgs.push_back("ZIP");
                }
                if (bBuildNsis) {
                    installerArgs.push_back("NSIS");
                }
                installerArgs.push_back("-SkipSdkBuild");

                RunBuilderCommand("Build installer", installerArgs, logStream, bAllSucceeded);
            }

            if (bSmokeTest) {
                const std::string smokeArchitecture = !architectures.empty() ? architectures.back() : std::string("win64");
                const std::string smokeConfiguration =
                    std::find(configurations.begin(), configurations.end(), "Release") != configurations.end()
                        ? std::string("Release")
                        : (!configurations.empty() ? configurations.front() : std::string("Release"));
                std::vector<std::string> smokeArgs {
                    "powershell",
                    "-NoProfile",
                    "-ExecutionPolicy",
                    "Bypass",
                    "-File",
                    "scripts/smoke_sdk_package.ps1",
                    "-Architecture",
                    smokeArchitecture,
                    "-Configuration",
                    smokeConfiguration
                };
                AppendGeneratorArguments(smokeArgs, selectedGenerator);
                RunBuilderCommand("Smoke test", smokeArgs, logStream, bAllSucceeded);
            }

            SetTextIfValid(logText, logStream.str());
            SetTextIfValid(commandPreview, bAllSucceeded ? "Build tasks finished." : "Build tasks failed.");
            button.SetDisabled(false);
        });
    }
}

class FGeneratedAppHostDelegate final : public ImWidgetV4::IApplicationHostDelegate
{
public:
    ImWidgetV4::FApplicationHostConfig GetHostConfig() const override
    {
        ImWidgetV4::FApplicationHostConfig config = GeneratedApp::BuildHostConfig();
        return config;
    }

    void ConfigureApplication(ImWidgetV4::ImApplication& application) override
    {
        GeneratedApp::ConfigureApplication(application);
        application.SetApplicationIcon(application.GetCoreIconBrush(ImWidgetV4::ECoreIcon::Package));
        ConfigureBuilderUi(application);
    }

    // Optional host overrides. Uncomment the functions you want to customize.
    // void ConfigureBackend(ImWidgetV4::ImApplicationBackend& backend) override
    // {
    //     (void)backend;
    // }

    // bool InitializeApplication(ImWidgetV4::ImApplication& application, ImWidgetV4::ImApplicationBackend& backend) override
    // {
    //     (void)application;
    //     (void)backend;
    //     return true;
    // }

    // void Tick(ImWidgetV4::ImApplication& application, const ImWidgetV4::FFrameInfo& frameInfo) override
    // {
    //     (void)application;
    //     (void)frameInfo;
    // }

    // bool OnCloseRequested(ImWidgetV4::ImApplication& application) override
    // {
    //     (void)application;
    //     return true;
    // }

    // void OnShutdown(ImWidgetV4::ImApplication& application) override
    // {
    //     (void)application;
    // }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FGeneratedAppHostDelegate>();
}

} // namespace ImWidgetV4
