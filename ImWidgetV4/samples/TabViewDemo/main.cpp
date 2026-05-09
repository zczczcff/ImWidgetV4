#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
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

class FTabViewDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "TabView Demo - ImWidgetV4";
        config.InitialWidth = 1220;
        config.InitialHeight = 760;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"TabViewDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        Application_ = &application;

        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(12.0f);

        auto title = std::make_shared<ImTextBlock>();
        title->SetText("TabView Demo");
        title->SetFontSize(28.0f);
        title->SetTextColor(FColor::White);
        root->AddChild(title, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto subtitle = std::make_shared<ImTextBlock>();
        subtitle->SetText("This demo shows editor-style document tabs with text-only and icon-labeled headers, dirty markers, close buttons, middle-click close, clipped-title tooltips, overflow navigation, and active-content routing.");
        subtitle->SetWrapText(true);
        subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
        root->AddChild(subtitle, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

        Status_ = std::make_shared<ImTextBlock>();
        Status_->SetText("Status: switch tabs, edit text inside a tab, or add/remove documents.");
        Status_->SetTextColor(FColor::FromBytes(160, 214, 190));
        root->AddChild(Status_, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

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

        TextTabs_ = std::make_shared<ImTabView>();
        TextTabs_->AddTab("Welcome", MakeTextDocument("Welcome", "This tab uses a simple text-only header and hosts a vertical document layout."));
        const int notesTab = TextTabs_->AddTab("Notes", MakeTextDocument("Notes", "Tabs keep their widget instances alive while inactive, but only the active one participates in the current frame chain."));
        const int logsTab = TextTabs_->AddTab("Logs", MakeScrollDocument("Build Logs"));
        const int graphTab = TextTabs_->AddTab("Graph", MakeTextDocument("Graph", "Extra tabs intentionally force overflow buttons into view when the demo window is narrow."));
        const int previewTab = TextTabs_->AddTab("Preview", MakeTextDocument("Preview", "Close buttons and dirty markers are meant for document-style workspaces."));
        TextTabs_->SetCloseActivationPolicy(ETabCloseActivationPolicy::MostRecentlyActive);
        TextTabs_->SetTabClosable(notesTab, true);
        TextTabs_->SetTabClosable(logsTab, true);
        TextTabs_->SetTabClosable(graphTab, true);
        TextTabs_->SetTabClosable(previewTab, true);
        TextTabs_->SetTabDirty(notesTab, true);
        TextTabs_->SetTabDirty(previewTab, true);
        TextTabs_->OnActiveTabChanged.AddLambda([this](ImTabView& sender, int index) {
            const FTabViewItem* item = sender.GetTab(index);
            Status_->SetText(item != nullptr
                ? "Status: activated text tab \"" + item->Title + "\""
                : "Status: text tab selection cleared");
        });
        TextTabs_->OnTabClosed.AddLambda([this](ImTabView&, int index) {
            Status_->SetText("Status: closed text tab at previous index " + std::to_string(index));
        });
        TextTabs_->OnTabDoubleClicked.AddLambda([this](ImTabView& sender, int index) {
            const FTabViewItem* item = sender.GetTab(index);
            if (item != nullptr) {
                Status_->SetText("Status: double-clicked text tab \"" + item->Title + "\"");
            }
        });
        root->AddChild(TextTabs_, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

        auto iconLabel = std::make_shared<ImTextBlock>();
        iconLabel->SetText("Icon Tabs");
        iconLabel->SetTextColor(FColor::FromBytes(255, 214, 102));
        root->AddChild(iconLabel, FMargin(18.0f, 8.0f, 18.0f, 0.0f));

        IconTabs_ = std::make_shared<ImTabView>();
        const int searchTab = IconTabs_->AddTab(
            "Search",
            application.GetCoreIconBrush(ECoreIcon::Search),
            MakeTextDocument("Search Workspace", "Icon headers are intended for editor-style workspaces and tool tabs."));
        const int settingsTab = IconTabs_->AddTab(
            "Settings",
            application.GetCoreIconBrush(ECoreIcon::Settings),
            MakeTextDocument("Project Settings", "This tab demonstrates icon + text layout inside the tab strip."));
        const int viewTab = IconTabs_->AddTab(
            "View",
            application.GetCoreIconBrush(ECoreIcon::View),
            MakeScrollDocument("Viewport Overlay Layers"));
        IconTabs_->SetTabClosable(settingsTab, true);
        IconTabs_->SetTabClosable(viewTab, true);
        IconTabs_->SetTabDirty(searchTab, true);
        IconTabs_->OnTabInvoked.AddLambda([this](ImTabView& sender, int index) {
            const FTabViewItem* item = sender.GetTab(index);
            if (item != nullptr) {
                Status_->SetText("Status: clicked icon tab \"" + item->Title + "\"");
            }
        });
        IconTabs_->OnTabDoubleClicked.AddLambda([this](ImTabView& sender, int index) {
            const FTabViewItem* item = sender.GetTab(index);
            if (item != nullptr) {
                Status_->SetText("Status: double-clicked icon tab \"" + item->Title + "\"");
            }
        });
        root->AddChildFill(IconTabs_, 1.0f, FMargin(18.0f, 0.0f, 18.0f, 18.0f));

        TextTabs_->OnTabContextMenuRequested.AddLambda([this](ImTabView&, int index, FVector2 position) {
            OpenTabContextMenu(TextTabs_, index, position);
        });
        IconTabs_->OnTabContextMenuRequested.AddLambda([this](ImTabView&, int index, FVector2 position) {
            OpenTabContextMenu(IconTabs_, index, position);
        });

        auto dynamicCounter = std::make_shared<int>(1);
        addTabButton->OnClicked.AddLambda([this, dynamicCounter](ImButton&) {
            const int documentNumber = ++(*dynamicCounter);
            const std::string title = "Draft " + std::to_string(documentNumber);
            const int index = TextTabs_->AddTab(
                title,
                MakeTextDocument(title, "This tab was added programmatically through the demo action row."));
            TextTabs_->SetTabClosable(index, true);
            TextTabs_->SetTabDirty(index, true);
            TextTabs_->SetActiveTab(index);
            Status_->SetText("Status: added document \"" + title + "\"");
        });

        removeTabButton->OnClicked.AddLambda([this](ImButton&) {
            const int activeIndex = TextTabs_->GetActiveTabIndex();
            const FTabViewItem* item = TextTabs_->GetTab(activeIndex);
            if (activeIndex >= 0 && item != nullptr) {
                const std::string title = item->Title;
                TextTabs_->RemoveTab(activeIndex);
                Status_->SetText("Status: removed document \"" + title + "\"");
            } else {
                Status_->SetText("Status: no active document to remove");
            }
        });

        application.SetRootWidget(root);
    }

private:
    void CloseTabContextMenu()
    {
        if (TabContextPopupWindow_) {
            if (TabContextPopupWindow_->IsOpen()) {
                Application_->GetWindowManager().CloseWindow(TabContextPopupWindow_);
            }
            TabContextPopupWindow_.reset();
        }
        TabContextMenu_.reset();
    }

    void OpenTabContextMenu(const std::shared_ptr<ImTabView>& tabView, int index, const FVector2& position)
    {
        const FTabViewItem* item = tabView->GetTab(index);
        if (item == nullptr || Application_ == nullptr) {
            return;
        }

        CloseTabContextMenu();

        auto menu = std::make_shared<ImPopupMenu>();
        FPopupMenuStyle menuStyle;
        menuStyle.MinDesiredSize = FVector2(180.0f, 36.0f);

        const bool canCloseThis = tabView->IsTabClosable(index);
        bool canCloseLeft = false;
        bool canCloseRight = false;
        bool canCloseOthers = false;
        bool canCloseAny = false;
        for (int tabIndex = 0; tabIndex < tabView->GetTabCount(); ++tabIndex) {
            if (tabView->IsTabClosable(tabIndex)) {
                canCloseAny = true;
                if (tabIndex < index) {
                    canCloseLeft = true;
                }
                if (tabIndex > index) {
                    canCloseRight = true;
                }
                if (tabIndex != index) {
                    canCloseOthers = true;
                }
            }
        }

        std::vector<FPopupMenuItem> items;
        items.push_back(FPopupMenuItem {
            "Close",
            Application_->GetCoreIconBrush(ECoreIcon::Remove),
            {},
            canCloseThis,
            false,
            [this, tabView, index]() {
                const FTabViewItem* current = tabView->GetTab(index);
                if (current != nullptr && tabView->IsTabClosable(index)) {
                    const std::string title = current->Title;
                    tabView->RemoveTab(index);
                    Status_->SetText("Status: closed tab \"" + title + "\" from context menu.");
                }
                CloseTabContextMenu();
            }
        });
        items.push_back(FPopupMenuItem {
            "Close Others",
            Application_->GetCoreIconBrush(ECoreIcon::Trash),
            {},
            canCloseOthers,
            false,
            [this, tabView, index]() {
                const FTabViewItem* current = tabView->GetTab(index);
                const std::string title = current != nullptr ? current->Title : std::string("Unknown");
                for (int tabIndex = tabView->GetTabCount() - 1; tabIndex >= 0; --tabIndex) {
                    if (tabIndex != index && tabView->IsTabClosable(tabIndex)) {
                        tabView->RemoveTab(tabIndex);
                    }
                }
                Status_->SetText("Status: kept only \"" + title + "\" among closable tabs.");
                CloseTabContextMenu();
            }
        });
        items.push_back(FPopupMenuItem {
            "Close Left",
            Application_->GetCoreIconBrush(ECoreIcon::ArrowUp),
            {},
            canCloseLeft,
            false,
            [this, tabView, index]() {
                int closedCount = 0;
                for (int tabIndex = index - 1; tabIndex >= 0; --tabIndex) {
                    if (tabView->IsTabClosable(tabIndex)) {
                        tabView->RemoveTab(tabIndex);
                        ++closedCount;
                    }
                }
                const FTabViewItem* current = tabView->GetTab(tabView->GetActiveTabIndex());
                const std::string title = current != nullptr ? current->Title : std::string("current tab");
                Status_->SetText("Status: closed " + std::to_string(closedCount) + " closable tab(s) to the left of \"" + title + "\".");
                CloseTabContextMenu();
            }
        });
        items.push_back(FPopupMenuItem {
            "Close Right",
            Application_->GetCoreIconBrush(ECoreIcon::ArrowDown),
            {},
            canCloseRight,
            false,
            [this, tabView, index]() {
                int closedCount = 0;
                for (int tabIndex = tabView->GetTabCount() - 1; tabIndex > index; --tabIndex) {
                    if (tabView->IsTabClosable(tabIndex)) {
                        tabView->RemoveTab(tabIndex);
                        ++closedCount;
                    }
                }
                const FTabViewItem* current = tabView->GetTab(tabView->GetActiveTabIndex());
                const std::string title = current != nullptr ? current->Title : std::string("current tab");
                Status_->SetText("Status: closed " + std::to_string(closedCount) + " closable tab(s) to the right of \"" + title + "\".");
                CloseTabContextMenu();
            }
        });
        items.push_back(FPopupMenuItem {
            "Close All Closable",
            Application_->GetCoreIconBrush(ECoreIcon::Trash),
            {},
            canCloseAny,
            false,
            [this, tabView]() {
                int closedCount = 0;
                for (int tabIndex = tabView->GetTabCount() - 1; tabIndex >= 0; --tabIndex) {
                    if (tabView->IsTabClosable(tabIndex)) {
                        tabView->RemoveTab(tabIndex);
                        ++closedCount;
                    }
                }
                Status_->SetText("Status: closed " + std::to_string(closedCount) + " closable tab(s).");
                CloseTabContextMenu();
            }
        });
        items.push_back(FPopupMenuItem {
            std::string(),
            FImageBrush(),
            {},
            true,
            true,
            {}
        });
        items.push_back(FPopupMenuItem {
            item->bDirty ? "Mark Clean" : "Mark Dirty",
            item->bDirty ? Application_->GetCoreIconBrush(ECoreIcon::Unlock) : Application_->GetCoreIconBrush(ECoreIcon::Lock),
            {},
            true,
            false,
            [this, tabView, index]() {
                const bool wasDirty = tabView->IsTabDirty(index);
                tabView->SetTabDirty(index, !wasDirty);
                const FTabViewItem* current = tabView->GetTab(index);
                if (current != nullptr) {
                    Status_->SetText(std::string("Status: tab \"") + current->Title + "\" marked " + (!wasDirty ? "dirty." : "clean."));
                }
                CloseTabContextMenu();
            }
        });

        menu->SetStyle(menuStyle);
        menu->SetItems(std::move(items));
        menu->OnItemInvoked.AddLambda([this](ImPopupMenu&, int) {
            CloseTabContextMenu();
        });

        const std::shared_ptr<ImWindow> parentWindow =
            Application_->GetWindowManager().FindWindowForWidget(std::static_pointer_cast<ImWidget>(tabView));
        if (!parentWindow) {
            return;
        }

        FPopupOptions popupOptions;
        popupOptions.Title = "TabContextMenu";
        popupOptions.Position = position;
        popupOptions.Size = menu->GetMinSize();
        popupOptions.RootWidget = menu;
        popupOptions.ParentWindow = parentWindow;
        popupOptions.Style.BackgroundColor = menuStyle.BackgroundColor;
        popupOptions.Style.InactiveBackgroundColor = menuStyle.BackgroundColor;
        popupOptions.Style.BorderColor = menuStyle.BorderColor;
        popupOptions.Style.ActiveBorderColor = menuStyle.BorderColor;
        popupOptions.Style.CornerRadius = menuStyle.CornerRadius;
        popupOptions.Style.BorderThickness = menuStyle.BorderThickness;
        popupOptions.Style.bDrawShadow = true;
        popupOptions.Style.ShadowColor = FColor(0.0f, 0.0f, 0.0f, 0.18f);
        popupOptions.Style.ShadowOffset = FVector2(0.0f, 10.0f);

        TabContextMenu_ = menu;
        TabContextPopupWindow_ = Application_->GetWindowManager().CreatePopup(popupOptions);
    }

    ImApplication* Application_ = nullptr;
    std::shared_ptr<ImTextBlock> Status_;
    std::shared_ptr<ImTabView> TextTabs_;
    std::shared_ptr<ImTabView> IconTabs_;
    std::shared_ptr<ImPopupMenu> TabContextMenu_;
    std::shared_ptr<ImWindow> TabContextPopupWindow_;
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FTabViewDemoHostDelegate>();
}

} // namespace ImWidgetV4
