#include "editor/EditorSession.h"
#include "editor/EditorShellHost.h"
#include "editor/EditorPaths.h"
#include "editor/EditorWorkspaceController.h"
#include "inspector/ReflectionDetailsView.h"
#include "palette/WidgetPaletteView.h"
#include "toolchains/EnvironmentProbe.h"
#include "tree/DocumentTreeViewBinder.h"

#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/PopupMenu.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include "../samples/DemoPaths.h"
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

std::filesystem::path GetEditorWorkspaceStatePath()
{
    const std::filesystem::path directory = Samples::GetDefaultSampleDataDirectory("editor");
    std::error_code errorCode;
    std::filesystem::create_directories(directory, errorCode);
    return directory / "workspace_state.json";
}

struct FEditorShellWidgets {
    std::shared_ptr<ImWidget> Root;
    std::shared_ptr<ImTitleBar> TitleBar;
    std::shared_ptr<EditorShellHost> ShellHost;
    std::shared_ptr<ImTabView> DocumentTabs;
    std::shared_ptr<ImTextOutlineView> ProjectView;
    std::shared_ptr<ImTextOutlineView> WidgetTreeView;
    std::shared_ptr<ImTextList> BuildOverviewText;
    std::shared_ptr<ReflectionDetailsView> DetailsView;
    std::shared_ptr<ImTextList> OutputText;
    std::shared_ptr<ImImage> TitleBarIcon;
    std::shared_ptr<ImTextBlock> TitleBarText;
    std::shared_ptr<ImTextBlock> TitleBarProfileStatusText;
    std::shared_ptr<ImButton> UndoButton;
    std::shared_ptr<ImButton> RedoButton;
};

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

FButtonStyle MakeTitleBarButtonStyle(bool bHighlighted = false)
{
    FButtonStyle style = FButtonStyle::CreatePrimary();
    const FColor textColor = FColor::FromBytes(232, 238, 246);
    const FColor baseColor = bHighlighted ? FColor::FromBytes(72, 104, 146, 116) : FColor(0.0f, 0.0f, 0.0f, 0.0f);
    style.Normal = FButtonStateStyle(baseColor, FColor(0.0f, 0.0f, 0.0f, 0.0f), textColor, 0.0f, 0.0f, false);
    style.Hovered = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 24), FColor(0.0f, 0.0f, 0.0f, 0.0f), textColor, 0.0f, 0.0f, false);
    style.Pressed = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 38), FColor(0.0f, 0.0f, 0.0f, 0.0f), textColor, 0.0f, 0.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(FColor(0.0f, 0.0f, 0.0f, 0.0f), FColor(0.0f, 0.0f, 0.0f, 0.0f), FColor::FromBytes(132, 140, 150), 0.0f, 0.0f, false);
    return style;
}

FButtonStyle MakeTitleBarIconButtonStyle()
{
    FButtonStyle style = FButtonStyle::CreatePrimary();
    const FColor transparent(0.0f, 0.0f, 0.0f, 0.0f);
    const FColor textColor = FColor::FromBytes(232, 238, 246);
    style.Normal = FButtonStateStyle(transparent, transparent, textColor, 0.0f, 0.0f, false);
    style.Hovered = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 18), transparent, textColor, 0.0f, 0.0f, false);
    style.Pressed = FButtonStateStyle(FColor::FromBytes(255, 255, 255, 30), transparent, textColor, 0.0f, 0.0f, false);
    style.Focused = style.Hovered;
    style.Disabled = FButtonStateStyle(transparent, transparent, FColor::FromBytes(132, 140, 150), 0.0f, 0.0f, false);
    return style;
}

std::shared_ptr<ImImage> MakeTitleBarIcon(const FImageBrush& brush, float size = 16.0f)
{
    auto image = std::make_shared<ImImage>();
    image->SetBrush(brush);
    image->SetDesiredSize(FVector2(size, size));
    image->SetBackgroundColor(FColor(0.0f, 0.0f, 0.0f, 0.0f));
    image->SetBorderColor(FColor(0.0f, 0.0f, 0.0f, 0.0f));
    image->SetBorderThickness(0.0f);
    image->SetCornerRadius(0.0f);
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
    button->SetStyle(MakeTitleBarIconButtonStyle());
    button->SetContent(MakeTitleBarIcon(brush, 16.0f));
    button->SetCompactMinSize(FVector2(30.0f, 28.0f));
    if (ImPaddingSlot* slot = button->GetContentSlot()) {
        slot->PaddingLeft = 7.0f;
        slot->PaddingRight = 7.0f;
        slot->PaddingTop = 6.0f;
        slot->PaddingBottom = 6.0f;
    }
    button->SetToolTipText(tooltip);
    return button;
}

void BindPopupMenuButton(
    ImApplication& application,
    const std::shared_ptr<FCompactTitleBarButton>& button,
    const std::function<std::vector<FPopupMenuItem>()>& itemBuilder)
{
    auto popupState = std::make_shared<FTitleBarPopupState>();
    popupState->Menu = std::make_shared<ImPopupMenu>();
    popupState->Menu->OnItemInvoked.AddLambda([popupState](ImPopupMenu&, int) {
        if (popupState->Window && popupState->Window->IsOpen()) {
            popupState->Window->Close();
        }
    });

    button->OnClicked.AddLambda([&application, button, popupState, itemBuilder](ImButton&) {
        if (popupState->Window && popupState->Window->IsOpen()) {
            popupState->Window->Close();
            return;
        }

        popupState->Menu->SetItems(itemBuilder());

        const FVector2 popupPosition = button->GetGeometry().Position + FVector2(0.0f, button->GetGeometry().Size.Y);
        const FVector2 popupSize = popupState->Menu->GetMinSize();

        if (!popupState->Window) {
            FPopupOptions popupOptions;
            popupOptions.Title = "TitleBarMenu";
            popupOptions.Position = popupPosition;
            popupOptions.Size = popupSize;
            popupOptions.RootWidget = popupState->Menu;
            popupOptions.ParentWindow = application.GetWindowManager().GetMainWindow();
            popupState->Window = application.GetWindowManager().CreatePopup(popupOptions);
        } else {
            popupState->Window->SetPosition(popupPosition);
            popupState->Window->SetSize(popupSize);
            popupState->Window->Open();
        }
    });
}

std::shared_ptr<ImTextBlock> MakePanelTitle(const std::string& text)
{
    auto title = std::make_shared<ImTextBlock>();
    title->SetText(text);
    title->SetFontSize(18.0f);
    title->SetTextColor(FColor::FromBytes(238, 242, 247));
    return title;
}

std::shared_ptr<ImTextBlock> MakePanelBody(const std::string& text, float fontSize = 14.0f)
{
    auto body = std::make_shared<ImTextBlock>();
    body->SetText(text);
    body->SetWrapText(false);
    body->SetFontSize(fontSize);
    body->SetTextColor(FColor::FromBytes(180, 190, 204));
    return body;
}

std::shared_ptr<ImTextList> MakePanelTextList(const std::vector<std::string>& items)
{
    auto list = std::make_shared<ImTextList>();
    FTextListStyle style = list->GetStyle();
    style.BackgroundColor = FColor::FromBytes(18, 23, 29);
    style.BorderColor = FColor::FromBytes(0, 0, 0, 0);
    style.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    style.TextColor = FColor::FromBytes(180, 190, 204);
    style.SelectionBackgroundColor = FColor::FromBytes(72, 104, 146, 148);
    style.Padding = FMargin(14.0f);
    style.MinDesiredSize = FVector2(0.0f, 120.0f);
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.LineSpacing = 1.1f;
    list->SetStyle(style);
    list->SetItems(items);
    return list;
}

std::shared_ptr<ImVerticalBox> MakeSimplePanel(const std::string& title, const std::string& bodyText)
{
    auto panel = std::make_shared<ImVerticalBox>();
    panel->SetSpacing(10.0f);
    panel->AddChild(MakePanelTitle(title), FMargin(14.0f, 14.0f, 14.0f, 14.0f));
    panel->AddChild(MakePanelBody(bodyText), FMargin(14.0f, 0.0f, 14.0f, 14.0f));
    return panel;
}

FTextOutlineViewStyle MakeDockOutlineStyle()
{
    FTextOutlineViewStyle style;
    style.Padding = FMargin(6.0f);
    style.RowPadding = FMargin(5.0f, 6.0f, 3.0f, 3.0f);
    style.MinDesiredSize = FVector2(220.0f, 180.0f);
    style.CornerRadius = 0.0f;
    style.BorderThickness = 0.0f;
    style.FontSize = 14.0f;
    style.RowHeight = 22.0f;
    style.IndentWidth = 16.0f;
    style.IndicatorSize = 9.0f;
    return style;
}

std::shared_ptr<ImWidget> BuildControlPalettePanel()
{
    auto host = std::make_shared<ImScrollBox>();
    FScrollBoxStyle style = host->GetStyle();
    style.Padding = FMargin(6.0f);
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    host->SetStyle(style);
    host->SetContent(BuildWidgetPaletteView());
    return host;
}

std::shared_ptr<ImTextOutlineView> BuildProjectViewPanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());
    return outline;
}

std::shared_ptr<ImWidget> BuildWidgetTreePanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());
    return outline;
}

std::shared_ptr<ImTextList> BuildBuildOverviewPanel()
{
    return MakePanelTextList({
        "No project loaded.",
        "",
        "Build/Toolchain overview will appear here."
    });
}

std::shared_ptr<ImWidget> BuildInitialDocumentRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("RootCanvas");
    canvas->SetDesiredSize(FVector2(1280.0f, 720.0f));

    auto title = std::make_shared<ImTextBlock>();
    title->SetName("TitleText");
    title->SetText("ImWidgetV4 Editor");
    title->SetFontSize(32.0f);
    title->SetWrapText(false);
    title->SetTextColor(FColor::FromBytes(235, 240, 248));
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(title, FVector2(0.08f, 0.08f))) {
        slot->SetAutoSize(true);
    }

    auto hint = std::make_shared<ImTextBlock>();
    hint->SetName("HintText");
    hint->SetText("Drag widgets from the left palette into the designer surface.");
    hint->SetFontSize(18.0f);
    hint->SetWrapText(false);
    hint->SetTextColor(FColor::FromBytes(162, 175, 191));
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(hint, FVector2(0.08f, 0.16f))) {
        slot->SetAutoSize(true);
    }

    auto button = std::make_shared<ImButton>();
    button->SetName("PrimaryButton");
    button->SetText("Action");
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(button, FVector2(0.08f, 0.28f))) {
        slot->SetAutoSize(true);
    }

    return canvas;
}

std::shared_ptr<ImTabView> BuildLeftDockTabs()
{
    auto tabView = std::make_shared<ImTabView>();
    tabView->SetSupportsKeyboardFocus(true);
    tabView->SetTabStripPlacement(ETabStripPlacement::Bottom);

    FTabViewStyle style = tabView->GetStyle();
    style.Padding = FMargin(0.0f);
    style.TabPadding = FMargin(4.0f, 4.0f, 2.0f, 2.0f);
    style.TabHeight = 20.0f;
    style.TabMinWidth = 64.0f;
    style.TabSpacing = 0.0f;
    style.FontSize = 16.0f;
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.BackgroundColor = FColor::FromBytes(22, 27, 33);
    style.TabStripBackgroundColor = FColor::FromBytes(27, 33, 41);
    style.TabColor = FColor::FromBytes(39, 45, 54);
    style.TabHoveredColor = FColor::FromBytes(52, 60, 71);
    style.TabPressedColor = FColor::FromBytes(33, 39, 47);
    style.ActiveTabColor = FColor::FromBytes(66, 94, 134);
    tabView->SetStyle(style);

    tabView->AddTab("Controls", BuildControlPalettePanel());
    auto projectView = BuildProjectViewPanel();
    tabView->AddTab("Project", projectView);
    auto buildOverview = BuildBuildOverviewPanel();
    tabView->AddTab("Build", buildOverview);
    tabView->AddTab("Widget Tree", BuildWidgetTreePanel());
    tabView->SetActiveTab(0);
    return tabView;
}

FEditorShellWidgets BuildEditorShell()
{
    FEditorShellWidgets shell;

    auto shellHost = std::make_shared<EditorShellHost>();
    auto rootLayout = std::make_shared<ImVerticalBox>();
    rootLayout->SetSpacing(0.0f);
    auto titleBar = std::make_shared<ImTitleBar>();
    FTitleBarStyle titleBarStyle = titleBar->GetStyle();
    titleBarStyle.Height = 24.0f;
    titleBarStyle.Padding = FMargin(4.0f, 0.0f, 0.0f, 0.0f);
    titleBarStyle.ItemSpacing = 4.0f;
    titleBarStyle.SystemButtonSize = 34.0f;
    titleBarStyle.MinDesiredSize = FVector2(0.0f, 24.0f);
    titleBar->SetStyle(titleBarStyle);

    auto titleIcon = MakeTitleBarIcon(FImageBrush(), 18.0f);
    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText("ImWidgetV4 Editor");
    titleText->SetFontSize(16.0f);
    titleText->SetWrapText(false);
    titleText->SetTextColor(FColor::FromBytes(238, 242, 247));

    auto titleBarProfileStatusText = std::make_shared<ImTextBlock>();
    titleBarProfileStatusText->SetText("");
    titleBarProfileStatusText->SetFontSize(12.0f);
    titleBarProfileStatusText->SetWrapText(false);
    titleBarProfileStatusText->SetTextColor(FColor::FromBytes(150, 160, 172));

    titleBar->AddLeadingItem(titleIcon);
    titleBar->AddLeadingItem(titleText);
    titleBar->AddLeadingItem(titleBarProfileStatusText);

    auto undoButton = MakeTitleBarIconButton(FImageBrush(), "Undo");
    auto redoButton = MakeTitleBarIconButton(FImageBrush(), "Redo");

    auto verticalShell = std::make_shared<ImVerticalSplitter>();
    verticalShell->SetSupportsKeyboardFocus(false);
    verticalShell->SetPartMinSize(0, 300.0f);

    auto topWorkspace = std::make_shared<ImHorizontalSplitter>();
    topWorkspace->SetSupportsKeyboardFocus(false);

    FHorizontalSplitterStyle horizontalStyle = topWorkspace->GetSplitterStyle();
    horizontalStyle.BarWidth = 5.0f;
    horizontalStyle.Color = FColor::FromBytes(44, 51, 61);
    horizontalStyle.HoveredColor = FColor::FromBytes(70, 82, 99);
    horizontalStyle.ActiveColor = FColor::FromBytes(103, 177, 255);
    topWorkspace->SetSplitterStyle(horizontalStyle);

    FVerticalSplitterStyle verticalStyle = verticalShell->GetSplitterStyle();
    verticalStyle.BarHeight = 5.0f;
    verticalStyle.Color = FColor::FromBytes(44, 51, 61);
    verticalStyle.HoveredColor = FColor::FromBytes(70, 82, 99);
    verticalStyle.ActiveColor = FColor::FromBytes(103, 177, 255);
    verticalShell->SetSplitterStyle(verticalStyle);

    auto leftDock = BuildLeftDockTabs();
    auto projectView = std::dynamic_pointer_cast<ImTextOutlineView>(leftDock->GetTab(1)->Content);
    auto buildOverviewText = std::dynamic_pointer_cast<ImTextList>(leftDock->GetTab(2)->Content);
    auto widgetTreeView = std::dynamic_pointer_cast<ImTextOutlineView>(leftDock->GetTab(3)->Content);

    auto documentTabs = std::make_shared<ImTabView>();
    documentTabs->SetSupportsKeyboardFocus(true);
    FTabViewStyle tabStyle = documentTabs->GetStyle();
    tabStyle.Padding = FMargin(0.0f);
    tabStyle.TabHeight = 36.0f;
    tabStyle.TabMinWidth = 150.0f;
    tabStyle.TabSpacing = 0.0f;
    tabStyle.BorderThickness = 0.0f;
    tabStyle.CornerRadius = 0.0f;
    tabStyle.TabStripBackgroundColor = FColor::FromBytes(27, 33, 41);
    tabStyle.BackgroundColor = FColor::FromBytes(18, 23, 29);
    tabStyle.ActiveTabColor = FColor::FromBytes(63, 90, 128);
    documentTabs->SetStyle(tabStyle);

    auto rightDock = std::make_shared<ImVerticalBox>();
    rightDock->SetSpacing(0.0f);
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    rightDock->AddChildFill(detailsView, 1.0f, FMargin(0.0f));

    topWorkspace->AddPart(leftDock, 0.22f, 240.0f);
    topWorkspace->AddPart(documentTabs, 0.56f, 420.0f);
    topWorkspace->AddPart(rightDock, 0.22f, 260.0f);

    auto bottomDock = std::make_shared<ImVerticalBox>();
    bottomDock->SetSpacing(0.0f);
    auto outputText = MakePanelTextList({"Booting editor session..."});
    bottomDock->AddChildFill(outputText, 1.0f, FMargin(0.0f, 0.0f, 0.0f, 0.0f));

    verticalShell->AddPart(topWorkspace, 0.78f, 360.0f);
    verticalShell->AddPart(bottomDock, 0.22f, 140.0f);

    shellHost->SetRootWidget(verticalShell);
    rootLayout->AddChild(titleBar, FMargin(0.0f));
    rootLayout->AddChildFill(shellHost, 1.0f, FMargin(0.0f));

    shell.Root = rootLayout;
    shell.TitleBar = titleBar;
    shell.ShellHost = shellHost;
    shell.DocumentTabs = documentTabs;
    shell.ProjectView = projectView;
    shell.WidgetTreeView = widgetTreeView;
    shell.BuildOverviewText = buildOverviewText;
    shell.DetailsView = detailsView;
    shell.OutputText = outputText;
    shell.TitleBarIcon = titleIcon;
    shell.TitleBarText = titleText;
    shell.TitleBarProfileStatusText = titleBarProfileStatusText;
    shell.UndoButton = undoButton;
    shell.RedoButton = redoButton;
    return shell;
}

std::vector<FApplicationMenuItem> BuildFileMenuItems(
    ImApplication& app,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    return {
        FApplicationMenuItem {"New", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->NewDocument();
            }
        }},
        FApplicationMenuItem {"Open...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenDocument(app);
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Save", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SaveDocument(app);
            }
        }},
        FApplicationMenuItem {"Save As...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SaveDocumentAs(app);
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Close", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->CloseActiveDocument(app);
            }
        }}
    };
}

std::vector<FApplicationMenuItem> BuildSimpleMenuItems(const std::string& menuName)
{
    std::vector<FApplicationMenuItem> items;
    items.push_back(FApplicationMenuItem {menuName + " Action", FImageBrush(), {}, true, false, []() {}});
    items.push_back(FApplicationMenuItem {"", FImageBrush(), {}, true, true, {}});
    items.push_back(FApplicationMenuItem {"Coming Soon", FImageBrush(), {}, false, false, {}});
    return items;
}

std::vector<FApplicationMenuItem> BuildEditMenuItems(const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    return {
        FApplicationMenuItem {"Cut", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->CutSelectedWidget();
            }
        }},
        FApplicationMenuItem {"Copy", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->CopySelectedWidget();
            }
        }},
        FApplicationMenuItem {"Paste", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->PasteCopiedWidget();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Undo", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->Undo();
            }
        }},
        FApplicationMenuItem {"Redo", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->Redo();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Duplicate", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->DuplicateSelectedWidget();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Coming Soon", {}, {}, false, false, {}}
    };
}

std::vector<FApplicationMenuItem> BuildBuildMenuItems(const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    const bool bHasProject = workspaceController && !workspaceController->GetProjectRoot().empty();
    const bool bBuildRunning = workspaceController && workspaceController->IsBuildTaskRunning();
    const std::string buildStatus = workspaceController ? workspaceController->GetBuildTaskStatusText() : std::string();
    std::vector<FApplicationMenuItem> items = {
        FApplicationMenuItem {
            bBuildRunning
                ? std::string("Status: ") + (buildStatus.empty() ? "Running..." : buildStatus)
                : std::string("Status: Idle"),
            {},
            {},
            false,
            false,
            {}
        },
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Configure Active Profile", {}, {}, bHasProject && !bBuildRunning, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->ConfigureProject();
            }
        }},
        FApplicationMenuItem {"Build Active Profile", {}, {}, bHasProject && !bBuildRunning, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->BuildProject();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}}
    };

    const std::string activeProfileName =
        workspaceController ? workspaceController->GetActiveBuildProfileName() : std::string();
    if (!activeProfileName.empty()) {
        items.push_back(FApplicationMenuItem {
            std::string("Active Profile: ") + activeProfileName,
            {},
            {},
            false,
            false,
            {}
        });

        const std::shared_ptr<EditorProject> project =
            workspaceController ? workspaceController->GetProject() : nullptr;
        if (project) {
            for (const FEditorBuildProfile& profile : project->GetBuildProfiles()) {
                const FEnvironmentProbeReport probeReport = EnvironmentProbe::Probe(profile);
                const std::string readinessLabel = probeReport.bReady ? "Ready" : "Needs Setup";
                std::vector<FApplicationMenuItem> profileSubItems = {
                    FApplicationMenuItem {
                        profile.Name == activeProfileName ? "Active Profile" : "Set Active Profile",
                        {},
                        {},
                        !bBuildRunning,
                        false,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->SetActiveBuildProfile(profileName);
                            }
                        }
                    },
                    FApplicationMenuItem {
                        "Configure This Profile",
                        {},
                        {},
                        !bBuildRunning,
                        false,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->ConfigureProject(profileName);
                            }
                        }
                    },
                    FApplicationMenuItem {
                        "Build This Profile",
                        {},
                        {},
                        !bBuildRunning,
                        false,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->BuildProject(profileName);
                            }
                        }
                    },
                    FApplicationMenuItem {
                        "Reveal Build Folder",
                        {},
                        {},
                        true,
                        false,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->RevealProjectBuildDirectory(profileName);
                            }
                        }
                    },
                    FApplicationMenuItem {"", {}, {}, true, true, {}},
                    FApplicationMenuItem {
                        "Target: " + GetTargetPlatformDisplayName(profile.TargetPlatform),
                        {},
                        {},
                        false,
                        false,
                        {}
                    },
                    FApplicationMenuItem {
                        "Configuration: " + profile.Configuration,
                        {},
                        {},
                        false,
                        false,
                        {}
                    },
                    FApplicationMenuItem {
                        "Probe: " + readinessLabel,
                        {},
                        {},
                        false,
                        false,
                        {}
                    }
                };

                items.push_back(FApplicationMenuItem {
                    (profile.Name == activeProfileName ? "[x] " : "[ ] ") + profile.Name + " [" + readinessLabel + "]",
                    {},
                    std::move(profileSubItems),
                    true,
                    false,
                    {}
                });
            }
        }

        items.push_back(FApplicationMenuItem {"", {}, {}, true, true, {}});
    }

    items.push_back(FApplicationMenuItem {"Reveal Build Folder", {}, {}, bHasProject, false, [workspaceController]() {
        if (workspaceController) {
            workspaceController->RevealProjectBuildDirectory();
        }
    }});

    items.push_back(FApplicationMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FApplicationMenuItem {
        bHasProject && !activeProfileName.empty()
            ? std::string("Profile: ") + activeProfileName
            : std::string("No active build profile"),
        {},
        {},
        false,
        false,
        {}
    });
    items.push_back(FApplicationMenuItem {
        bHasProject
            ? std::string("Build workflow uses saved project profile settings")
            : std::string("Build directory not available"),
        {},
        {},
        false,
        false,
        {}
    });

    return items;
}

std::vector<FPopupMenuItem> BuildProjectMenuItems(
    ImApplication& app,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    return {
        FPopupMenuItem {"New App Project...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->NewAppProject(app);
            }
        }},
        FPopupMenuItem {"Open App Project...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenAppProject(app);
            }
        }},
        FPopupMenuItem {"", {}, {}, true, true, {}},
        FPopupMenuItem {"Open Project Root...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SelectProjectRoot(app);
            }
        }},
        FPopupMenuItem {"Project Settings...", {}, {}, workspaceController && workspaceController->GetProject(), false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenProjectSettings(app);
            }
        }},
        FPopupMenuItem {"", {}, {}, true, true, {}},
        FPopupMenuItem {"Generate C++...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->GenerateActiveDocumentCpp(app);
            }
        }},
        FPopupMenuItem {"", {}, {}, true, true, {}},
        FPopupMenuItem {"New UI Document...", {}, {}, workspaceController && !workspaceController->GetProjectRoot().empty(), false, [&app, workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->CreateDocumentInDirectory(app, workspaceController->GetProjectRoot());
            }
        }},
        FPopupMenuItem {"New Folder", {}, {}, workspaceController && !workspaceController->GetProjectRoot().empty(), false, [&app, workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->CreateFolderInDirectory(app, workspaceController->GetProjectRoot());
            }
        }},
        FPopupMenuItem {"Refresh Project Tree", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RefreshProjectTree();
            }
        }},
        FPopupMenuItem {"Reveal Project Root", {}, {}, workspaceController && !workspaceController->GetProjectRoot().empty(), false, [workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->RevealProjectItemInExplorer(workspaceController->GetProjectRoot());
            }
        }},
        FPopupMenuItem {"", {}, {}, true, true, {}},
        FPopupMenuItem {
            workspaceController && !workspaceController->GetProjectRoot().empty()
                ? workspaceController->GetProjectRoot().string()
                : std::string("Project root not configured"),
            {},
            {},
            false,
            false,
            {}
        }
    };
}

void RebuildEditorTitleBar(
    ImApplication& app,
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    if (!shell.TitleBar) {
        return;
    }

    shell.TitleBar->ClearLeadingItems();
    shell.TitleBar->ClearTrailingItems();

    if (shell.TitleBarIcon) {
        shell.TitleBarIcon->SetBrush(app.GetApplicationIcon());
        shell.TitleBar->AddLeadingItem(shell.TitleBarIcon);
    }

    if (shell.TitleBarText) {
        const std::string projectLabel = workspaceController && !workspaceController->GetProjectRoot().empty()
            ? workspaceController->GetProjectRoot().filename().string()
            : std::string("ImWidgetV4 Editor");
        shell.TitleBarText->SetText(projectLabel);
        shell.TitleBar->AddLeadingItem(shell.TitleBarText);
    }

    if (shell.TitleBarProfileStatusText) {
        shell.TitleBar->AddLeadingItem(shell.TitleBarProfileStatusText);
    }

    auto fileButton = MakeTitleBarTextButton("File");
    BindPopupMenuButton(app, fileButton, [&app, workspaceController]() {
        return BuildFileMenuItems(app, workspaceController);
    });
    shell.TitleBar->AddLeadingItem(fileButton);

    auto editButton = MakeTitleBarTextButton("Edit");
    BindPopupMenuButton(app, editButton, [workspaceController]() {
        return BuildEditMenuItems(workspaceController);
    });
    shell.TitleBar->AddLeadingItem(editButton);

    auto projectButton = MakeTitleBarTextButton("Project");
    BindPopupMenuButton(app, projectButton, [&app, workspaceController]() {
        return BuildProjectMenuItems(app, workspaceController);
    });
    shell.TitleBar->AddLeadingItem(projectButton);

    auto buildButton = MakeTitleBarTextButton("Build");
    BindPopupMenuButton(app, buildButton, [workspaceController]() {
        return BuildBuildMenuItems(workspaceController);
    });
    shell.TitleBar->AddLeadingItem(buildButton);

    auto viewButton = MakeTitleBarTextButton("View");
    BindPopupMenuButton(app, viewButton, []() {
        return BuildSimpleMenuItems("View");
    });
    shell.TitleBar->AddLeadingItem(viewButton);

    if (shell.UndoButton) {
        shell.UndoButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Undo, FColor::FromBytes(210, 219, 232)), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.UndoButton);
    }
    if (shell.RedoButton) {
        shell.RedoButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Redo, FColor::FromBytes(210, 219, 232)), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.RedoButton);
    }

    auto searchButton = MakeTitleBarIconButton(app.GetCoreIconBrush(ECoreIcon::Search, FColor::FromBytes(210, 219, 232)), "Search");
    BindPopupMenuButton(app, searchButton, []() {
        return BuildSimpleMenuItems("Search");
    });
    shell.TitleBar->AddLeadingItem(searchButton);
}

void UpdateEditorTitleBarActions(
    ImApplication& app,
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    const bool bCanUndo = workspaceController && workspaceController->GetActiveSession() && workspaceController->GetActiveSession()->CanUndo();
    const bool bCanRedo = workspaceController && workspaceController->GetActiveSession() && workspaceController->GetActiveSession()->CanRedo();

    if (shell.UndoButton) {
        shell.UndoButton->SetDisabled(!bCanUndo);
        shell.UndoButton->SetStyle(MakeTitleBarIconButtonStyle());
        shell.UndoButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Undo, bCanUndo ? FColor::FromBytes(235, 242, 250) : FColor::FromBytes(132, 140, 150)),
            16.0f));
    }

    if (shell.RedoButton) {
        shell.RedoButton->SetDisabled(!bCanRedo);
        shell.RedoButton->SetStyle(MakeTitleBarIconButtonStyle());
        shell.RedoButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Redo, bCanRedo ? FColor::FromBytes(235, 242, 250) : FColor::FromBytes(132, 140, 150)),
            16.0f));
    }

    if (shell.TitleBarProfileStatusText) {
        std::string statusText = "No active build profile";
        FColor statusColor = FColor::FromBytes(150, 160, 172);

        if (workspaceController) {
            const std::string activeProfileName = workspaceController->GetActiveBuildProfileName();
            if (!activeProfileName.empty()) {
                statusText = activeProfileName;
                if (workspaceController->IsBuildTaskRunning()) {
                    const std::string buildStatus = workspaceController->GetBuildTaskStatusText();
                    statusText += " | " + (buildStatus.empty() ? std::string("Running") : buildStatus);
                    statusColor = FColor::FromBytes(103, 177, 255);
                } else if (const std::shared_ptr<EditorProject> project = workspaceController->GetProject()) {
                    if (const FEditorBuildProfile* profile = project->FindBuildProfile(activeProfileName)) {
                        const FEnvironmentProbeReport probeReport = EnvironmentProbe::Probe(*profile);
                        if (probeReport.bReady) {
                            statusText += " | Ready";
                            statusColor = FColor::FromBytes(125, 204, 138);
                        } else {
                            statusText += " | Needs Setup";
                            statusColor = FColor::FromBytes(230, 184, 104);
                        }
                    }
                }
            }
        }

        shell.TitleBarProfileStatusText->SetText(statusText);
        shell.TitleBarProfileStatusText->SetTextColor(statusColor);
    }
}

void UpdateBuildOverviewPanel(
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    if (!shell.BuildOverviewText) {
        return;
    }

    std::vector<std::string> lines;
    if (!workspaceController || !workspaceController->GetProject()) {
        lines = {
            "No project loaded.",
            "",
            "Open or create an app project to inspect toolchain readiness."
        };
        shell.BuildOverviewText->SetItems(lines);
        return;
    }

    const std::string activeProfileName = workspaceController->GetActiveBuildProfileName();
    const FEnvironmentProbeReport probeReport = workspaceController->GetActiveBuildProfileProbeReport();
    lines.push_back("Active Profile: " + (activeProfileName.empty() ? std::string("None") : activeProfileName));
    lines.push_back("Build Status: " + (workspaceController->IsBuildTaskRunning()
        ? workspaceController->GetBuildTaskStatusText()
        : std::string("Idle")));
    lines.push_back("Probe Ready: " + std::string(probeReport.bReady ? "Yes" : "No"));
    lines.push_back("");

    if (probeReport.Items.empty()) {
        lines.push_back("No probe data available.");
    } else {
        lines.push_back("Environment Probe:");
        for (const FEnvironmentProbeItem& item : probeReport.Items) {
            lines.push_back("  " + item.Label + " [" + ToDisplayString(item.Status) + "]");
            if (!item.Details.empty()) {
                lines.push_back("    " + item.Details);
            }
        }
    }

    const std::vector<std::string> outputLines = workspaceController->GetOutputLines();
    if (!outputLines.empty()) {
        lines.push_back("");
        lines.push_back("Recent Output:");
        const std::size_t previewCount = std::min<std::size_t>(5, outputLines.size());
        for (std::size_t index = outputLines.size() - previewCount; index < outputLines.size(); ++index) {
            lines.push_back("  " + outputLines[index]);
        }
    }

    shell.BuildOverviewText->SetItems(lines);
}

} // namespace

class FEditorApplicationHostDelegate : public IApplicationHostDelegate
{
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "ImWidgetV4 Editor";
        config.InitialWidth = 1440;
        config.InitialHeight = 900;
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath("ImWidgetV4Editor.ini");
        config.bUseCustomHostChrome = true;
        return config;
    }

    void ConfigureApplication(ImApplication& app) override
    {
        BoundApplication_ = &app;
        const std::filesystem::path defaultWorkspaceDirectory = GetDefaultEditorWorkspaceDirectory();
        std::error_code currentPathError;
        std::filesystem::current_path(defaultWorkspaceDirectory, currentPathError);

        WorkspaceStatePath_ = GetEditorWorkspaceStatePath();

        Shell_ = BuildEditorShell();
        WorkspaceController_ = std::make_shared<EditorWorkspaceController>(&BuildInitialDocumentRoot);
        Shell_.ShellHost->SetWorkspaceController(WorkspaceController_);
        WorkspaceController_->SetOnProjectStateChanged([this, appPtr = &app, weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)]() {
            auto lockedWorkspace = weakWorkspace.lock();
            if (appPtr == nullptr || !lockedWorkspace) {
                return;
            }

            RebuildEditorTitleBar(*appPtr, Shell_, lockedWorkspace);
            UpdateBuildOverviewPanel(Shell_, lockedWorkspace);
        });
        WorkspaceController_->SetOnExitRequested([appPtr = &app]() {
            if (appPtr == nullptr) {
                return;
            }

            if (ImApplicationBackend* backend = appPtr->GetBackend()) {
                backend->RequestClose();
            }
        });
        WorkspaceController_->Bind(
            Shell_.ShellHost,
            Shell_.DocumentTabs,
            Shell_.ProjectView,
            Shell_.WidgetTreeView,
            Shell_.DetailsView,
            Shell_.OutputText);
        if (!WorkspaceController_->LoadWorkspaceState(WorkspaceStatePath_)) {
            WorkspaceController_->SetProjectRoot(defaultWorkspaceDirectory);
        }

        app.SetApplicationTitle("ImWidgetV4 Editor");
        app.SetApplicationIcon(app.GetCoreIconBrush(ECoreIcon::Settings));
        if (Shell_.UndoButton) {
            Shell_.UndoButton->SetToolTipText("Undo");
            Shell_.UndoButton->OnClicked.AddLambda([weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                if (auto lockedWorkspace = weakWorkspace.lock()) {
                    lockedWorkspace->Undo();
                }
            });
        }
        if (Shell_.RedoButton) {
            Shell_.RedoButton->SetToolTipText("Redo");
            Shell_.RedoButton->OnClicked.AddLambda([weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                if (auto lockedWorkspace = weakWorkspace.lock()) {
                    lockedWorkspace->Redo();
                }
            });
        }
        RebuildEditorTitleBar(app, Shell_, WorkspaceController_);
        UpdateEditorTitleBarActions(app, Shell_, WorkspaceController_);
        UpdateBuildOverviewPanel(Shell_, WorkspaceController_);
        app.SetRootWidget(Shell_.Root);
    }

    bool InitializeApplication(ImApplication&, ImApplicationBackend&) override
    {
        return true;
    }

    void Tick(ImApplication&, const FFrameInfo&) override
    {
        if (WorkspaceController_) {
            WorkspaceController_->TickBackgroundTasks();
        }
        if (WorkspaceController_) {
            // Keep undo/redo availability responsive without rebuilding the full shell tree.
            // The title text is updated on project state changes via RebuildEditorTitleBar.
            if (BoundApplication_ != nullptr) {
                UpdateEditorTitleBarActions(*BoundApplication_, Shell_, WorkspaceController_);
            }
            UpdateBuildOverviewPanel(Shell_, WorkspaceController_);
        }
    }

    bool OnCloseRequested(ImApplication& app) override
    {
        if (!WorkspaceController_) {
            return true;
        }

        return WorkspaceController_->RequestApplicationClose(app);
    }

    void OnShutdown(ImApplication&) override
    {
        if (WorkspaceController_) {
            WorkspaceController_->SaveWorkspaceState(WorkspaceStatePath_);
        }
    }

private:
    FEditorShellWidgets Shell_;
    std::shared_ptr<EditorWorkspaceController> WorkspaceController_;
    std::filesystem::path WorkspaceStatePath_;
    ImApplication* BoundApplication_ = nullptr;
};

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FEditorApplicationHostDelegate>();
}

} // namespace ImWidgetV4
