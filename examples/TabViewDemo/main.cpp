#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <Windows.h>
#include <memory>
#include <string>

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImWidget> MakeTextDocument(const std::string& title, const std::string& description)
{
    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(10.0f);

    auto heading = std::make_shared<ImTextBlock>();
    heading->SetText(title);
    heading->SetFontSize(22.0f);
    heading->SetTextColor(FColor::White);
    root->AddChild(heading);

    auto paragraph = std::make_shared<ImTextBlock>();
    paragraph->SetText(description);
    paragraph->SetWrapText(true);
    paragraph->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(paragraph);

    auto input = std::make_shared<ImEditableText>();
    input->SetHintText("Type notes for this tab...");
    root->AddChild(input);

    return root;
}

std::shared_ptr<ImWidget> MakeScrollDocument(const std::string& title)
{
    auto content = std::make_shared<ImVerticalBox>();
    content->SetSpacing(8.0f);

    auto heading = std::make_shared<ImTextBlock>();
    heading->SetText(title);
    heading->SetFontSize(20.0f);
    heading->SetTextColor(FColor::White);
    content->AddChild(heading);

    for (int index = 0; index < 18; ++index) {
        auto row = std::make_shared<ImTextBlock>();
        row->SetText("Scrollable row " + std::to_string(index + 1) + " for the active document tab.");
        row->SetTextColor(FColor::FromBytes(214, 222, 234));
        content->AddChild(row);
    }

    auto scrollBox = std::make_shared<ImScrollBox>();
    scrollBox->SetContent(content);
    return scrollBox;
}

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"TabView Demo - ImWidgetV4",
        1220,
        760
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"TabViewDemo.ini"));
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("TabView Demo");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("This demo shows editor-style document tabs with text-only and icon-labeled headers, dirty markers, close buttons, overflow navigation, and active-content routing.");
    subtitle->SetWrapText(true);
    subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(subtitle, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto status = std::make_shared<ImTextBlock>();
    status->SetText("Status: switch tabs, edit text inside a tab, or add/remove documents.");
    status->SetTextColor(FColor::FromBytes(160, 214, 190));
    root->AddChild(status, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto actionLabel = std::make_shared<ImTextBlock>();
    actionLabel->SetText("Document Actions");
    actionLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(actionLabel, FMargin(18.0f, 4.0f, 18.0f, 0.0f));

    auto actions = std::make_shared<ImHorizontalBox>();
    actions->SetSpacing(8.0f);
    auto addTabButton = std::make_shared<ImButton>();
    addTabButton->SetText("Add Document");
    auto removeTabButton = std::make_shared<ImButton>();
    removeTabButton->SetText("Remove Active");
    actions->AddChild(addTabButton);
    actions->AddChild(removeTabButton);
    root->AddChild(actions, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto textTabs = std::make_shared<ImTabView>();
    textTabs->AddTab("Welcome", MakeTextDocument("Welcome", "This tab uses a simple text-only header and hosts a vertical document layout."));
    const int notesTab = textTabs->AddTab("Notes", MakeTextDocument("Notes", "Tabs keep their widget instances alive while inactive, but only the active one participates in the current frame chain."));
    const int logsTab = textTabs->AddTab("Logs", MakeScrollDocument("Build Logs"));
    const int graphTab = textTabs->AddTab("Graph", MakeTextDocument("Graph", "Extra tabs intentionally force overflow buttons into view when the demo window is narrow."));
    const int previewTab = textTabs->AddTab("Preview", MakeTextDocument("Preview", "Close buttons and dirty markers are meant for document-style workspaces."));
    textTabs->SetTabClosable(notesTab, true);
    textTabs->SetTabClosable(logsTab, true);
    textTabs->SetTabClosable(graphTab, true);
    textTabs->SetTabClosable(previewTab, true);
    textTabs->SetTabDirty(notesTab, true);
    textTabs->SetTabDirty(previewTab, true);
    textTabs->OnActiveTabChanged.AddLambda([status](ImTabView& sender, int index) {
        const FTabViewItem* item = sender.GetTab(index);
        status->SetText(item != nullptr
            ? "Status: activated text tab \"" + item->Title + "\""
            : "Status: text tab selection cleared");
    });
    textTabs->OnTabClosed.AddLambda([status](ImTabView&, int index) {
        status->SetText("Status: closed text tab at previous index " + std::to_string(index));
    });
    root->AddChild(textTabs, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto iconLabel = std::make_shared<ImTextBlock>();
    iconLabel->SetText("Icon Tabs");
    iconLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
    root->AddChild(iconLabel, FMargin(18.0f, 8.0f, 18.0f, 0.0f));

    auto iconTabs = std::make_shared<ImTabView>();
    const int searchTab = iconTabs->AddTab(
        "Search",
        app->GetCoreIconBrush(ECoreIcon::Search),
        MakeTextDocument("Search Workspace", "Icon headers are intended for editor-style workspaces and tool tabs."));
    const int settingsTab = iconTabs->AddTab(
        "Settings",
        app->GetCoreIconBrush(ECoreIcon::Settings),
        MakeTextDocument("Project Settings", "This tab demonstrates icon + text layout inside the tab strip."));
    const int viewTab = iconTabs->AddTab(
        "View",
        app->GetCoreIconBrush(ECoreIcon::View),
        MakeScrollDocument("Viewport Overlay Layers"));
    iconTabs->SetTabClosable(settingsTab, true);
    iconTabs->SetTabClosable(viewTab, true);
    iconTabs->SetTabDirty(searchTab, true);
    iconTabs->OnTabInvoked.AddLambda([status](ImTabView& sender, int index) {
        const FTabViewItem* item = sender.GetTab(index);
        if (item != nullptr) {
            status->SetText("Status: clicked icon tab \"" + item->Title + "\"");
        }
    });
    root->AddChildFill(iconTabs, 1.0f, FMargin(18.0f, 0.0f, 18.0f, 18.0f));

    auto dynamicCounter = std::make_shared<int>(1);
    addTabButton->OnClicked.AddLambda([textTabs, status, dynamicCounter](ImButton&) {
        const int documentNumber = ++(*dynamicCounter);
        const std::string title = "Draft " + std::to_string(documentNumber);
        const int index = textTabs->AddTab(
            title,
            MakeTextDocument(title, "This tab was added programmatically through the demo action row."));
        textTabs->SetTabClosable(index, true);
        textTabs->SetTabDirty(index, true);
        textTabs->SetActiveTab(index);
        status->SetText("Status: added document \"" + title + "\"");
    });

    removeTabButton->OnClicked.AddLambda([textTabs, status](ImButton&) {
        const int activeIndex = textTabs->GetActiveTabIndex();
        const FTabViewItem* item = textTabs->GetTab(activeIndex);
        if (activeIndex >= 0 && item != nullptr) {
            const std::string title = item->Title;
            textTabs->RemoveTab(activeIndex);
            status->SetText("Status: removed document \"" + title + "\"");
        } else {
            status->SetText("Status: no active document to remove");
        }
    });

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();
    return 0;
}
