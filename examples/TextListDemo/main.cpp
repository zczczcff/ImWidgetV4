#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>
#include <vector>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

std::vector<std::string> BuildSampleLog()
{
    return {
        "[09:42:11] AssetRegistry: refresh completed for 128 assets. Long descriptions wrap automatically so dense logs remain readable without horizontal scrolling.",
        "[09:42:12] Snapshot: baseline capture queued.\nSecond line keeps its explicit newline and still participates in text selection.",
        "[09:42:14] Input: drag across the text to create a selection, then press Ctrl+C after the list has focus.",
        "[09:42:16] Theme: accent preset switched to Slate Blue. The log list keeps per-item colors and shared multi-line layout.",
        "[09:42:21] Automation: retained-mode controls are now wired through the same application input routing used by buttons, sliders, and text boxes.",
        "[09:42:27] Note: scrollbar dragging and wheel scrolling remain available while the list supports long wrapped entries."
    };
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"TextList Demo - ImWidgetV4",
        980,
        720
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"TextListDemo.ini"));
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("Interactive Text List Demo");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("This version follows the old ImWidget multi-line text behavior more closely: wrapped log items, drag-to-select text, Ctrl+C copy, per-item color, and a retained scrollbar.");
    subtitle->SetWrapText(true);
    subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(subtitle, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto toolbar = std::make_shared<ImHorizontalBox>();
    toolbar->SetSpacing(10.0f);

    auto focusButton = std::make_shared<ImButton>();
    focusButton->SetText("Focus List");
    toolbar->AddChild(focusButton);

    auto jumpButton = std::make_shared<ImButton>();
    jumpButton->SetText("Jump To Last");
    toolbar->AddChild(jumpButton);

    auto appendButton = std::make_shared<ImButton>();
    appendButton->SetText("Append Log");
    toolbar->AddChild(appendButton);

    auto resetButton = std::make_shared<ImButton>();
    resetButton->SetText("Reset Log");
    resetButton->SetStyle(FButtonStyle::CreatePrimary());
    toolbar->AddChild(resetButton);

    root->AddChild(toolbar, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto status = std::make_shared<ImTextBlock>();
    status->SetText("Status: drag across the text to select it, then press Ctrl+C.");
    status->SetWrapText(true);
    status->SetTextColor(FColor::FromBytes(160, 214, 190));
    root->AddChild(status, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto list = std::make_shared<ImTextList>();
    list->SetItems(BuildSampleLog());
    list->SetItemColor(0, FColor::FromBytes(255, 214, 102));
    list->SetItemColor(1, FColor::FromBytes(199, 226, 255));
    root->AddChildFill(list, 1.0f, FMargin(18.0f, 0.0f, 18.0f, 18.0f));

    focusButton->OnClicked.AddLambda([&](ImButton&) {
        app->SetKeyboardFocus(list);
        status->SetText("Status: list focused. Use Ctrl+A to select all, Ctrl+C to copy.");
    });

    jumpButton->OnClicked.AddLambda([&](ImButton&) {
        const int targetIndex = static_cast<int>(list->GetItems().size()) - 1;
        if (targetIndex >= 0 && list->ScrollToItem(targetIndex, true)) {
            status->SetText("Status: jumped to the last log entry.");
        }
    });

    appendButton->OnClicked.AddLambda([&](ImButton&) {
        const int nextIndex = static_cast<int>(list->GetItems().size()) + 1;
        list->AddItem("[09:43:" + std::to_string(10 + nextIndex) + "] Runtime: a new wrapped log entry was appended to exercise incremental growth and scrolling.");
        list->SetItemColor(static_cast<int>(list->GetItems().size()) - 1, FColor::FromBytes(238, 241, 245));
        status->SetText("Status: appended one more log entry.");
    });

    resetButton->OnClicked.AddLambda([&](ImButton&) {
        list->SetItems(BuildSampleLog());
        list->SetItemColor(0, FColor::FromBytes(255, 214, 102));
        list->SetItemColor(1, FColor::FromBytes(199, 226, 255));
        list->SetScrollOffset(0.0f);
        list->ClearSelection();
        status->SetText("Status: log reset and selection cleared.");
    });

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();

    return 0;
}
