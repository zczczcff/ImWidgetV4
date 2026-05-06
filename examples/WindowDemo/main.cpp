#include <Windows.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <filesystem>
#include <string>

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImVerticalBox> MakeWindowStack(const std::string& titleText, const std::string& bodyText)
{
    auto layout = std::make_shared<ImVerticalBox>();
    layout->SetSpacing(10.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText(titleText);
    title->SetFontSize(20.0f);
    title->SetTextColor(FColor::FromBytes(255, 214, 102));
    layout->AddChild(title, FMargin(12.0f, 12.0f, 12.0f, 0.0f));

    auto body = std::make_shared<ImTextBlock>();
    body->SetText(bodyText);
    body->SetWrapText(true);
    body->SetTextColor(FColor::FromBytes(220, 227, 235));
    layout->AddChild(body, FMargin(12.0f, 0.0f, 12.0f, 0.0f));

    return layout;
}

std::string FormatDialogResultMessage(const std::string& label, const FPathDialogResult& result)
{
    switch (result.Code) {
    case EPathDialogResultCode::Accepted:
        return "Status: " + label + " = " + result.Path.string();
    case EPathDialogResultCode::Cancelled:
        return "Status: " + label + " cancelled.";
    case EPathDialogResultCode::Unsupported:
        return "Status: " + label + " unsupported by the current backend.";
    case EPathDialogResultCode::Error:
        return "Status: " + label + " failed: " + result.ErrorMessage;
    default:
        return "Status: " + label + " returned an unknown result.";
    }
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Window Demo - ImWidgetV4",
        1280,
        760
    );
    backend->SetUseCustomHostChrome(true);

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"WindowDemo.ini"));
    backend->SetApplication(app.get());
    app->SetApplicationTitle("Window Demo - ImWidgetV4");
    app->SetApplicationIcon(app->GetCoreIconBrush(ECoreIcon::Settings));

    auto mainRoot = std::make_shared<ImVerticalBox>();
    mainRoot->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("Window Management Demo");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    mainRoot->AddChild(title, FMargin(24.0f, 20.0f, 24.0f, 0.0f));

    auto body = std::make_shared<ImTextBlock>();
    body->SetText("This demo uses the new V4 window manager. Drag floating windows by the title bar, open popup and modal windows, and verify that the modal blocks interaction behind it.");
    body->SetWrapText(true);
    body->SetTextColor(FColor::FromBytes(214, 222, 234));
    mainRoot->AddChild(body, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto popupButton = std::make_shared<ImButton>();
    popupButton->SetText("Toggle Popup");
    popupButton->SetStyle(FButtonStyle::CreatePrimary());
    mainRoot->AddChild(popupButton, FMargin(24.0f, 4.0f, 24.0f, 0.0f));

    auto modalButton = std::make_shared<ImButton>();
    modalButton->SetText("Open Modal");
    mainRoot->AddChild(modalButton, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto openFileButton = std::make_shared<ImButton>();
    openFileButton->SetText("Open File Dialog");
    mainRoot->AddChild(openFileButton, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto openFolderButton = std::make_shared<ImButton>();
    openFolderButton->SetText("Open Folder Dialog");
    mainRoot->AddChild(openFolderButton, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto saveFileButton = std::make_shared<ImButton>();
    saveFileButton->SetText("Save File Dialog");
    mainRoot->AddChild(saveFileButton, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    auto status = std::make_shared<ImTextBlock>();
    status->SetText("Status: floating windows are ready.");
    status->SetTextColor(FColor::FromBytes(160, 214, 190));
    status->SetWrapText(true);
    mainRoot->AddChild(status, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

    app->SetRootWidget(mainRoot);

    FWindowOptions toolsOptions;
    toolsOptions.Title = "Tools";
    toolsOptions.Position = FVector2(850.0f, 72.0f);
    toolsOptions.Size = FVector2(300.0f, 220.0f);
    auto toolsRoot = MakeWindowStack(
        "Tools",
        "This floating window demonstrates retained-mode content hosted inside a top-level V4 window.");
    auto toolSlider = std::make_shared<ImSlider>();
    toolSlider->SetRange(0.0f, 100.0f);
    toolSlider->SetValue(36.0f);
    toolsRoot->AddChild(toolSlider, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    toolsOptions.RootWidget = toolsRoot;
    app->GetWindowManager().CreateWindow(toolsOptions);

    FWindowOptions inspectorOptions;
    inspectorOptions.Title = "Inspector";
    inspectorOptions.Position = FVector2(820.0f, 330.0f);
    inspectorOptions.Size = FVector2(340.0f, 250.0f);
    auto inspectorRoot = MakeWindowStack(
        "Inspector",
        "This window mixes checkbox and editable text controls to show normal input routing inside floating windows.");
    auto checkbox = std::make_shared<ImCheckBox>();
    checkbox->SetLabel("Lock controls");
    inspectorRoot->AddChild(checkbox, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    auto editable = std::make_shared<ImEditableText>();
    editable->SetHintText("Type something here...");
    inspectorRoot->AddChild(editable, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    inspectorOptions.RootWidget = inspectorRoot;
    app->GetWindowManager().CreateWindow(inspectorOptions);

    std::shared_ptr<ImWindow> popupWindow;
    std::shared_ptr<ImWindow> modalWindow;

    const auto togglePopup = [&]() {
        if (popupWindow && popupWindow->IsOpen()) {
            popupWindow->Close();
            status->SetText("Status: popup closed.");
            return;
        }

        if (!popupWindow) {
            FPopupOptions popupOptions;
            popupOptions.Title = "Quick Popup";
            popupOptions.Position = FVector2(220.0f, 170.0f);
            popupOptions.Size = FVector2(260.0f, 150.0f);
            popupOptions.RootWidget = MakeWindowStack(
                "Quick Popup",
                "Click outside this popup to close the entire popup chain.");
            popupWindow = app->GetWindowManager().CreatePopup(popupOptions);
        } else {
            popupWindow->Open();
        }

        status->SetText("Status: popup opened.");
    };

    const auto openModal = [&]() {
        if (modalWindow && modalWindow->IsOpen()) {
            return;
        }

        if (!modalWindow) {
            FPopupOptions modalOptions;
            modalOptions.Title = "Modal Dialog";
            modalOptions.Position = FVector2(420.0f, 210.0f);
            modalOptions.Size = FVector2(420.0f, 220.0f);

            auto modalRoot = MakeWindowStack(
                "Modal Dialog",
                "While this modal is open, background windows do not receive pointer input.");
            auto closeModal = std::make_shared<ImButton>();
            closeModal->SetText("Close Modal");
            closeModal->SetStyle(FButtonStyle::CreatePrimary());
            modalRoot->AddChild(closeModal, FMargin(12.0f, 0.0f, 12.0f, 12.0f));

            modalOptions.RootWidget = modalRoot;
            modalWindow = app->GetWindowManager().CreateModal(modalOptions);

            closeModal->OnClicked.AddLambda([&](ImButton&) {
                if (modalWindow) {
                    modalWindow->Close();
                    status->SetText("Status: modal closed.");
                }
            });
        } else {
            modalWindow->Open();
        }

        status->SetText("Status: modal opened.");
    };

    const auto openFileDialog = [&]() {
        FOpenFileDialogOptions options;
        options.Title = "Open a Demo File";
        options.InitialDirectory = std::filesystem::current_path();
        options.Filters = {
            FFileDialogFilter {"Text Files", {"*.txt", "*.md", "*.json"}},
            FFileDialogFilter {"Images", {"*.png", "*.jpg", "*.bmp"}},
            FFileDialogFilter {"All Files", {"*.*"}}
        };
        options.DefaultFilterIndex = 0;
        status->SetText(FormatDialogResultMessage("open file", app->OpenFileDialog(options)));
    };

    const auto openFolderDialog = [&]() {
        FOpenFolderDialogOptions options;
        options.Title = "Choose a Demo Folder";
        options.InitialDirectory = std::filesystem::current_path();
        status->SetText(FormatDialogResultMessage("open folder", app->OpenFolderDialog(options)));
    };

    const auto saveFileDialog = [&]() {
        FSaveFileDialogOptions options;
        options.Title = "Save a Demo File";
        options.InitialDirectory = std::filesystem::current_path();
        options.DefaultFileName = "window_demo_output";
        options.DefaultExtension = "txt";
        options.Filters = {
            FFileDialogFilter {"Text Files", {"*.txt"}},
            FFileDialogFilter {"JSON Files", {"*.json"}},
            FFileDialogFilter {"All Files", {"*.*"}}
        };
        options.DefaultFilterIndex = 0;
        options.bPromptOverwrite = true;
        status->SetText(FormatDialogResultMessage("save file", app->SaveFileDialog(options)));
    };

    popupButton->OnClicked.AddLambda([&](ImButton&) { togglePopup(); });
    modalButton->OnClicked.AddLambda([&](ImButton&) { openModal(); });
    openFileButton->OnClicked.AddLambda([&](ImButton&) { openFileDialog(); });
    openFolderButton->OnClicked.AddLambda([&](ImButton&) { openFolderDialog(); });
    saveFileButton->OnClicked.AddLambda([&](ImButton&) { saveFileDialog(); });

    checkbox->OnCheckStateChanged.AddLambda([&](ImCheckBox&, bool checked) {
        editable->SetDisabled(checked);
        status->SetText(checked ? "Status: inspector text box disabled." : "Status: inspector text box enabled.");
    });

    std::vector<FApplicationMenuItem> fileMenuItems;
    fileMenuItems.push_back(FApplicationMenuItem {
        "Toggle Popup",
        app->GetCoreIconBrush(ECoreIcon::Folder),
        {},
        true,
        false,
        [&]() { togglePopup(); }
    });
    fileMenuItems.push_back(FApplicationMenuItem {
        "Open Modal",
        app->GetCoreIconBrush(ECoreIcon::View),
        {},
        true,
        false,
        [&]() { openModal(); }
    });
    fileMenuItems.push_back(FApplicationMenuItem {
        "Open File Dialog",
        app->GetCoreIconBrush(ECoreIcon::File),
        {},
        true,
        false,
        [&]() { openFileDialog(); }
    });
    fileMenuItems.push_back(FApplicationMenuItem {
        "Open Folder Dialog",
        app->GetCoreIconBrush(ECoreIcon::Folder),
        {},
        true,
        false,
        [&]() { openFolderDialog(); }
    });
    fileMenuItems.push_back(FApplicationMenuItem {
        "Save File Dialog",
        app->GetCoreIconBrush(ECoreIcon::Save),
        {},
        true,
        false,
        [&]() { saveFileDialog(); }
    });
    fileMenuItems.push_back(FApplicationMenuItem {
        std::string(),
        FImageBrush(),
        {},
        true,
        true,
        {}
    });
    fileMenuItems.push_back(FApplicationMenuItem {
        "Exit Demo",
        app->GetCoreIconBrush(ECoreIcon::Trash),
        {},
        true,
        false,
        [&]() { backend->RequestClose(); }
    });
    app->AddTitleBarTabMenu("File", std::move(fileMenuItems));

    std::vector<FApplicationMenuItem> toolsMenuItems;
    toolsMenuItems.push_back(FApplicationMenuItem {
        "Enable Inspector Input",
        app->GetCoreIconBrush(ECoreIcon::Unlock),
        {},
        true,
        false,
        [&]() {
            checkbox->SetChecked(false);
            editable->SetDisabled(false);
            status->SetText("Status: inspector text box enabled.");
        }
    });
    std::vector<FApplicationMenuItem> inspectorSubMenuItems;
    inspectorSubMenuItems.push_back(FApplicationMenuItem {
        "Disable Inspector Input",
        app->GetCoreIconBrush(ECoreIcon::Lock),
        {},
        true,
        false,
        [&]() {
            checkbox->SetChecked(true);
            editable->SetDisabled(true);
            status->SetText("Status: inspector text box disabled.");
        }
    });
    inspectorSubMenuItems.push_back(FApplicationMenuItem {
        "Show Status Hint",
        app->GetCoreIconBrush(ECoreIcon::Search),
        {},
        true,
        false,
        [&]() {
            status->SetText("Status: inspector submenu invoked.");
        }
    });
    toolsMenuItems.push_back(FApplicationMenuItem {
        "Inspector",
        app->GetCoreIconBrush(ECoreIcon::Settings),
        inspectorSubMenuItems,
        true,
        false,
        {}
    });
    app->AddTitleBarTabMenu("Tools", std::move(toolsMenuItems));

    std::vector<FApplicationMenuItem> infoMenuItems;
    infoMenuItems.push_back(FApplicationMenuItem {
        "Host chrome tabs are active",
        FImageBrush(),
        {},
        false,
        false,
        {}
    });
    infoMenuItems.push_back(FApplicationMenuItem {
        "Show status hint",
        app->GetCoreIconBrush(ECoreIcon::Search),
        {},
        true,
        false,
        [&]() {
            status->SetText("Status: title-bar menu invoked.");
        }
    });
    app->AddTitleBarTabMenu(app->GetCoreIconBrush(ECoreIcon::View), std::move(infoMenuItems));

    backend->Run();
    backend->Shutdown();
    return 0;
}
