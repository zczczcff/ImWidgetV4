// Stable user entry translation unit.
// Platform entry points are provided by the selected ImWidgetV4 app host target.

#include "AppProjectConfig.h"
#include "MainView.h"
#include "TitleBarView.h"

#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/CoreIcon.h>
#include <imwidgetv4/platform/PlatformProcess.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>

#include <algorithm>
#include <filesystem>
#include <memory>
#include <set>
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
    BindAtLeastOneChecked(win64, win32);
    BindAtLeastOneChecked(debug, release);

    auto toolchain = mainView->FindWidgetAs<ImWidgetV4::ImComboBox>("ToolchainComboBox");
    if (toolchain) {
        const std::vector<std::string> options = DetectToolsetOptions();
        toolchain->SetItems(options);
        toolchain->SetSelectedIndex(options.empty() ? -1 : 0);
    }

    auto commandPreview = mainView->FindWidgetAs<ImWidgetV4::ImTextBlock>("CommandPreviewText");
    if (commandPreview) {
        commandPreview->SetText("package_sdk.ps1 -Architectures win32,win64 -Configurations Debug,Release");
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
