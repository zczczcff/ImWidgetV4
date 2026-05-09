#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/Slider.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <filesystem>
#include <memory>
#include <string>

using namespace ImWidgetV4;

namespace {

class FCompactTitleBarButton : public ImButton {
public:
    FCompactTitleBarButton()
        : ImButton()
    {
    }

    void SetCompactMinSize(const FVector2& size)
    {
        MinSize_ = size;
        Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    }

    FVector2 GetMinSize() const override
    {
        const auto& children = GetChildren();
        const ImPaddingSlot* slot = const_cast<FCompactTitleBarButton*>(this)->GetContentSlot();
        if (!children.empty() && slot != nullptr) {
            FVector2 contentMinSize = children[0]->GetMinSize();
            contentMinSize.X += slot->PaddingLeft + slot->PaddingRight;
            contentMinSize.Y += slot->PaddingTop + slot->PaddingBottom;
            return FVector2(
                std::max(contentMinSize.X, MinSize_.X),
                std::max(contentMinSize.Y, MinSize_.Y));
        }

        return MinSize_;
    }

private:
    FVector2 MinSize_ {0.0f, 28.0f};
};

struct FTitleBarPopupState {
    std::shared_ptr<ImPopupMenu> Menu;
    std::shared_ptr<ImWindow> Window;
};

FButtonStyle MakeTitleBarButtonStyle()
{
    FButtonStyle style = FButtonStyle::CreatePrimary();
    const FColor textColor = FColor::FromBytes(232, 238, 246);
    style.Normal = FButtonStateStyle(FColor(0.0f, 0.0f, 0.0f, 0.0f), FColor(0.0f, 0.0f, 0.0f, 0.0f), textColor, 0.0f, 4.0f, false);
    style.Hovered = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 24), FColor(0.0f, 0.0f, 0.0f, 0.0f), textColor, 0.0f, 4.0f, false);
    style.Pressed = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 38), FColor(0.0f, 0.0f, 0.0f, 0.0f), textColor, 0.0f, 4.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(FColor(0.0f, 0.0f, 0.0f, 0.0f), FColor(0.0f, 0.0f, 0.0f, 0.0f), FColor::FromBytes(132, 140, 150), 0.0f, 4.0f, false);
    return style;
}

std::shared_ptr<ImImage> MakeTitleBarIcon(const FImageBrush& brush, float size = 16.0f)
{
    auto image = std::make_shared<ImImage>();
    image->SetBrush(brush);
    image->SetDesiredSize(FVector2(size, size));
    return image;
}

std::shared_ptr<FCompactTitleBarButton> MakeTitleBarTextButton(const std::string& text)
{
    auto button = std::make_shared<FCompactTitleBarButton>();
    button->SetStyle(MakeTitleBarButtonStyle());
    button->SetText(text);
    button->SetCompactMinSize(FVector2(0.0f, 28.0f));
    if (ImPaddingSlot* slot = button->GetContentSlot()) {
        slot->PaddingLeft = 12.0f;
        slot->PaddingRight = 12.0f;
        slot->PaddingTop = 5.0f;
        slot->PaddingBottom = 5.0f;
    }
    return button;
}

std::shared_ptr<FCompactTitleBarButton> MakeTitleBarIconButton(const FImageBrush& brush, const std::string& tooltip)
{
    auto button = std::make_shared<FCompactTitleBarButton>();
    button->SetStyle(MakeTitleBarButtonStyle());
    button->SetContent(MakeTitleBarIcon(brush, 16.0f));
    button->SetCompactMinSize(FVector2(28.0f, 28.0f));
    if (ImPaddingSlot* slot = button->GetContentSlot()) {
        slot->PaddingLeft = 6.0f;
        slot->PaddingRight = 6.0f;
        slot->PaddingTop = 6.0f;
        slot->PaddingBottom = 6.0f;
    }
    button->SetToolTipText(tooltip);
    return button;
}

void BindPopupMenuButton(
    ImApplication& application,
    const std::shared_ptr<FCompactTitleBarButton>& button,
    const std::vector<FPopupMenuItem>& items)
{
    auto popupState = std::make_shared<FTitleBarPopupState>();
    popupState->Menu = std::make_shared<ImPopupMenu>();
    popupState->Menu->SetItems(items);
    popupState->Menu->OnItemInvoked.AddLambda([popupState](ImPopupMenu&, int) {
        if (popupState->Window && popupState->Window->IsOpen()) {
            popupState->Window->Close();
        }
    });

    button->OnClicked.AddLambda([&application, button, popupState](ImButton&) {
        if (popupState->Window && popupState->Window->IsOpen()) {
            popupState->Window->Close();
            return;
        }

        popupState->Menu->SetItems(popupState->Menu->GetItems());

        if (!popupState->Window) {
            FPopupOptions popupOptions;
            popupOptions.Title = "TitleBarMenu";
            popupOptions.Position = button->GetGeometry().Position + FVector2(0.0f, button->GetGeometry().Size.Y);
            popupOptions.Size = popupState->Menu->GetMinSize();
            popupOptions.RootWidget = popupState->Menu;
            popupOptions.ParentWindow = application.GetWindowManager().GetMainWindow();
            popupState->Window = application.GetWindowManager().CreatePopup(popupOptions);
        } else {
            popupState->Window->SetPosition(button->GetGeometry().Position + FVector2(0.0f, button->GetGeometry().Size.Y));
            popupState->Window->SetSize(popupState->Menu->GetMinSize());
            popupState->Window->Open();
        }
    });
}

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

class FWindowDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Window Demo - ImWidgetV4";
        config.InitialWidth = 1280;
        config.InitialHeight = 760;
        config.bUseCustomHostChrome = true;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"WindowDemo.ini");
#endif
        return config;
    }

    void ConfigureBackend(ImApplicationBackend& backend) override
    {
        Backend_ = &backend;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        Application_ = &application;
        application.SetApplicationTitle("Window Demo - ImWidgetV4");
        application.SetApplicationIcon(application.GetCoreIconBrush(ECoreIcon::Settings));

        auto mainRoot = std::make_shared<ImVerticalBox>();
        mainRoot->SetSpacing(0.0f);

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

        Status_ = std::make_shared<ImTextBlock>();
        Status_->SetText("Status: floating windows are ready.");
        Status_->SetTextColor(FColor::FromBytes(160, 214, 190));
        Status_->SetWrapText(true);
        mainRoot->AddChild(Status_, FMargin(24.0f, 0.0f, 24.0f, 0.0f));

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
        application.GetWindowManager().CreateWindow(toolsOptions);

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
        application.GetWindowManager().CreateWindow(inspectorOptions);

        const auto togglePopup = [this]() {
            TogglePopup();
        };
        const auto openModal = [this]() {
            OpenModal();
        };
        const auto openFileDialog = [this]() {
            FOpenFileDialogOptions options;
            options.Title = "Open a Demo File";
            options.InitialDirectory = std::filesystem::current_path();
            options.Filters = {
                FFileDialogFilter {"Text Files", {"*.txt", "*.md", "*.json"}},
                FFileDialogFilter {"Images", {"*.png", "*.jpg", "*.bmp"}},
                FFileDialogFilter {"All Files", {"*.*"}}
            };
            options.DefaultFilterIndex = 0;
            Status_->SetText(FormatDialogResultMessage("open file", Application_->OpenFileDialog(options)));
        };
        const auto openFolderDialog = [this]() {
            FOpenFolderDialogOptions options;
            options.Title = "Choose a Demo Folder";
            options.InitialDirectory = std::filesystem::current_path();
            Status_->SetText(FormatDialogResultMessage("open folder", Application_->OpenFolderDialog(options)));
        };
        const auto saveFileDialog = [this]() {
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
            Status_->SetText(FormatDialogResultMessage("save file", Application_->SaveFileDialog(options)));
        };

        popupButton->OnClicked.AddLambda([togglePopup](ImButton&) { togglePopup(); });
        modalButton->OnClicked.AddLambda([openModal](ImButton&) { openModal(); });
        openFileButton->OnClicked.AddLambda([openFileDialog](ImButton&) { openFileDialog(); });
        openFolderButton->OnClicked.AddLambda([openFolderDialog](ImButton&) { openFolderDialog(); });
        saveFileButton->OnClicked.AddLambda([saveFileDialog](ImButton&) { saveFileDialog(); });

        checkbox->OnCheckStateChanged.AddLambda([this, editable](ImCheckBox&, bool checked) {
            editable->SetDisabled(checked);
            Status_->SetText(checked ? "Status: inspector text box disabled." : "Status: inspector text box enabled.");
        });

        auto titleBar = std::make_shared<ImTitleBar>();
        FTitleBarStyle titleBarStyle = titleBar->GetStyle();
        titleBarStyle.Height = 38.0f;
        titleBarStyle.Padding = FMargin(10.0f, 6.0f, 10.0f, 6.0f);
        titleBarStyle.ItemSpacing = 4.0f;
        titleBarStyle.SystemButtonSize = 34.0f;
        titleBarStyle.MinDesiredSize = FVector2(0.0f, 38.0f);
        titleBar->SetStyle(titleBarStyle);

        auto appIcon = MakeTitleBarIcon(application.GetApplicationIcon(), 18.0f);
        auto appTitle = std::make_shared<ImTextBlock>();
        appTitle->SetText("Window Demo");
        appTitle->SetFontSize(16.0f);
        appTitle->SetWrapText(false);
        appTitle->SetTextColor(FColor::FromBytes(238, 242, 247));
        titleBar->AddLeadingItem(appIcon);
        titleBar->AddLeadingItem(appTitle);

        std::vector<FPopupMenuItem> fileMenuItems;
        fileMenuItems.push_back(FPopupMenuItem {
            "Toggle Popup",
            application.GetCoreIconBrush(ECoreIcon::Folder),
            {},
            true,
            false,
            [togglePopup]() { togglePopup(); }
        });
        fileMenuItems.push_back(FPopupMenuItem {
            "Open Modal",
            application.GetCoreIconBrush(ECoreIcon::View),
            {},
            true,
            false,
            [openModal]() { openModal(); }
        });
        fileMenuItems.push_back(FPopupMenuItem {
            "Open File Dialog",
            application.GetCoreIconBrush(ECoreIcon::File),
            {},
            true,
            false,
            [openFileDialog]() { openFileDialog(); }
        });
        fileMenuItems.push_back(FPopupMenuItem {
            "Open Folder Dialog",
            application.GetCoreIconBrush(ECoreIcon::Folder),
            {},
            true,
            false,
            [openFolderDialog]() { openFolderDialog(); }
        });
        fileMenuItems.push_back(FPopupMenuItem {
            "Save File Dialog",
            application.GetCoreIconBrush(ECoreIcon::Save),
            {},
            true,
            false,
            [saveFileDialog]() { saveFileDialog(); }
        });
        fileMenuItems.push_back(FPopupMenuItem {
            std::string(),
            FImageBrush(),
            {},
            true,
            true,
            {}
        });
        fileMenuItems.push_back(FPopupMenuItem {
            "Exit Demo",
            application.GetCoreIconBrush(ECoreIcon::Trash),
            {},
            true,
            false,
            [this]() {
                if (Backend_ != nullptr) {
                    Backend_->RequestClose();
                }
            }
        });
        auto fileMenuButton = MakeTitleBarTextButton("File");
        BindPopupMenuButton(application, fileMenuButton, fileMenuItems);
        titleBar->AddLeadingItem(fileMenuButton);

        std::vector<FPopupMenuItem> toolsMenuItems;
        toolsMenuItems.push_back(FPopupMenuItem {
            "Enable Inspector Input",
            application.GetCoreIconBrush(ECoreIcon::Unlock),
            {},
            true,
            false,
            [this, checkbox, editable]() {
                checkbox->SetChecked(false);
                editable->SetDisabled(false);
                Status_->SetText("Status: inspector text box enabled.");
            }
        });
        std::vector<FPopupMenuItem> inspectorSubMenuItems;
        inspectorSubMenuItems.push_back(FPopupMenuItem {
            "Disable Inspector Input",
            application.GetCoreIconBrush(ECoreIcon::Lock),
            {},
            true,
            false,
            [this, checkbox, editable]() {
                checkbox->SetChecked(true);
                editable->SetDisabled(true);
                Status_->SetText("Status: inspector text box disabled.");
            }
        });
        inspectorSubMenuItems.push_back(FPopupMenuItem {
            "Show Status Hint",
            application.GetCoreIconBrush(ECoreIcon::Search),
            {},
            true,
            false,
            [this]() {
                Status_->SetText("Status: inspector submenu invoked.");
            }
        });
        toolsMenuItems.push_back(FPopupMenuItem {
            "Inspector",
            application.GetCoreIconBrush(ECoreIcon::Settings),
            inspectorSubMenuItems,
            true,
            false,
            {}
        });
        auto toolsMenuButton = MakeTitleBarTextButton("Tools");
        BindPopupMenuButton(application, toolsMenuButton, toolsMenuItems);
        titleBar->AddLeadingItem(toolsMenuButton);

        std::vector<FPopupMenuItem> infoMenuItems;
        infoMenuItems.push_back(FPopupMenuItem {
            "Host chrome tabs are active",
            FImageBrush(),
            {},
            false,
            false,
            {}
        });
        infoMenuItems.push_back(FPopupMenuItem {
            "Show status hint",
            application.GetCoreIconBrush(ECoreIcon::Search),
            {},
            true,
            false,
            [this]() {
                Status_->SetText("Status: title-bar menu invoked.");
            }
        });
        auto infoMenuButton = MakeTitleBarIconButton(application.GetCoreIconBrush(ECoreIcon::View), "Info");
        BindPopupMenuButton(application, infoMenuButton, infoMenuItems);
        titleBar->AddLeadingItem(infoMenuButton);

        auto popupActionButton = MakeTitleBarTextButton("Popup");
        popupActionButton->OnClicked.AddLambda([togglePopup](ImButton&) { togglePopup(); });
        titleBar->AddTrailingItem(popupActionButton);

        auto modalActionButton = MakeTitleBarTextButton("Modal");
        modalActionButton->OnClicked.AddLambda([openModal](ImButton&) { openModal(); });
        titleBar->AddTrailingItem(modalActionButton);

        mainRoot->InsertChildAt(0, titleBar);
        application.SetRootWidget(mainRoot);
    }

private:
    void TogglePopup()
    {
        if (PopupWindow_ && PopupWindow_->IsOpen()) {
            PopupWindow_->Close();
            Status_->SetText("Status: popup closed.");
            return;
        }

        if (!PopupWindow_) {
            FPopupOptions popupOptions;
            popupOptions.Title = "Quick Popup";
            popupOptions.Position = FVector2(220.0f, 170.0f);
            popupOptions.Size = FVector2(260.0f, 150.0f);
            popupOptions.RootWidget = MakeWindowStack(
                "Quick Popup",
                "Click outside this popup to close the entire popup chain.");
            PopupWindow_ = Application_->GetWindowManager().CreatePopup(popupOptions);
        } else {
            PopupWindow_->Open();
        }

        Status_->SetText("Status: popup opened.");
    }

    void OpenModal()
    {
        if (ModalWindow_ && ModalWindow_->IsOpen()) {
            return;
        }

        if (!ModalWindow_) {
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
            ModalWindow_ = Application_->GetWindowManager().CreateModal(modalOptions);

            closeModal->OnClicked.AddLambda([this](ImButton&) {
                if (ModalWindow_) {
                    ModalWindow_->Close();
                    Status_->SetText("Status: modal closed.");
                }
            });
        } else {
            ModalWindow_->Open();
        }

        Status_->SetText("Status: modal opened.");
    }

    ImApplication* Application_ = nullptr;
    ImApplicationBackend* Backend_ = nullptr;
    std::shared_ptr<ImTextBlock> Status_;
    std::shared_ptr<ImWindow> PopupWindow_;
    std::shared_ptr<ImWindow> ModalWindow_;
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FWindowDemoHostDelegate>();
}

} // namespace ImWidgetV4
