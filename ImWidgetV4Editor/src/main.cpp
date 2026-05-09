#include "editor/EditorSession.h"
#include "editor/EditorShellHost.h"
#include "editor/EditorPaths.h"
#include "editor/EditorWorkspaceController.h"
#include "inspector/ReflectionDetailsView.h"
#include "palette/WidgetPaletteView.h"
#include "tree/DocumentTreeViewBinder.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include "../samples/DemoPaths.h"
#include <Windows.h>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

std::filesystem::path GetEditorWorkspaceStatePath()
{
    const std::filesystem::path directory = std::filesystem::path(L"C:\\ImWidgetV4\\editor");
    std::error_code errorCode;
    std::filesystem::create_directories(directory, errorCode);
    return directory / "workspace_state.json";
}

struct FEditorShellWidgets {
    std::shared_ptr<ImWidget> Root;
    std::shared_ptr<EditorShellHost> ShellHost;
    std::shared_ptr<ImTabView> DocumentTabs;
    std::shared_ptr<ImTextOutlineView> ProjectView;
    std::shared_ptr<ImTextOutlineView> WidgetTreeView;
    std::shared_ptr<ReflectionDetailsView> DetailsView;
    std::shared_ptr<ImTextList> OutputText;
};

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
    tabView->AddTab("Widget Tree", BuildWidgetTreePanel());
    tabView->SetActiveTab(0);
    return tabView;
}

FEditorShellWidgets BuildEditorShell()
{
    FEditorShellWidgets shell;

    auto shellHost = std::make_shared<EditorShellHost>();
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
    auto widgetTreeView = std::dynamic_pointer_cast<ImTextOutlineView>(leftDock->GetTab(2)->Content);

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

    shell.Root = shellHost;
    shell.ShellHost = shellHost;
    shell.DocumentTabs = documentTabs;
    shell.ProjectView = projectView;
    shell.WidgetTreeView = widgetTreeView;
    shell.DetailsView = detailsView;
    shell.OutputText = outputText;
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
    return {
        FApplicationMenuItem {"Configure Project", {}, {}, bHasProject, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->ConfigureProject();
            }
        }},
        FApplicationMenuItem {"Build Debug", {}, {}, bHasProject, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->BuildProject();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Reveal Build Folder", {}, {}, bHasProject, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RevealProjectBuildDirectory();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {
            bHasProject
                ? std::string("Build Dir: ") + (workspaceController->GetProjectRoot() / "build" / "win32-debug").string()
                : std::string("Build directory not available"),
            {},
            {},
            false,
            false,
            {}
        }
    };
}

void RebuildTitleBarMenus(ImApplication& app, const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    app.ClearTitleBarTabMenus();
    app.AddTitleBarTabMenu("File", BuildFileMenuItems(app, workspaceController));
    app.AddTitleBarTabMenu("Edit", BuildEditMenuItems(workspaceController));
    app.AddTitleBarTabMenu("Project", {
        FApplicationMenuItem {"New App Project...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->NewAppProject(app);
            }
        }},
        FApplicationMenuItem {"Open App Project...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenAppProject(app);
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Open Project Root...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SelectProjectRoot(app);
            }
        }},
        FApplicationMenuItem {"Generate C++...", {}, {}, true, false, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->GenerateActiveDocumentCpp(app);
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"New UI Document...", {}, {}, workspaceController && !workspaceController->GetProjectRoot().empty(), false, [&app, workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->CreateDocumentInDirectory(app, workspaceController->GetProjectRoot());
            }
        }},
        FApplicationMenuItem {"New Folder", {}, {}, workspaceController && !workspaceController->GetProjectRoot().empty(), false, [&app, workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->CreateFolderInDirectory(app, workspaceController->GetProjectRoot());
            }
        }},
        FApplicationMenuItem {"Refresh Project Tree", {}, {}, true, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RefreshProjectTree();
            }
        }},
        FApplicationMenuItem {"Reveal Project Root", {}, {}, workspaceController && !workspaceController->GetProjectRoot().empty(), false, [workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->RevealProjectItemInExplorer(workspaceController->GetProjectRoot());
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {
            workspaceController && !workspaceController->GetProjectRoot().empty()
                ? workspaceController->GetProjectRoot().string()
                : std::string("Project root not configured"),
            {},
            {},
            false,
            false,
            {}
        }
    });
    app.AddTitleBarTabMenu("Build", BuildBuildMenuItems(workspaceController));
    app.AddTitleBarTabMenu("View", BuildSimpleMenuItems("View"));
    app.AddTitleBarTabMenu(app.GetCoreIconBrush(ECoreIcon::Search), BuildSimpleMenuItems("Search"));
}

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    const std::filesystem::path defaultWorkspaceDirectory = GetDefaultEditorWorkspaceDirectory();
    std::error_code currentPathError;
    std::filesystem::current_path(defaultWorkspaceDirectory, currentPathError);

    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"ImWidgetV4 Editor",
        1440,
        900);
    backend->SetUseCustomHostChrome(true);

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Failed to initialize editor backend.", L"ImWidgetV4 Editor", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Samples::GetDefaultSampleImGuiIniPath(L"ImWidgetV4Editor.ini"));
    backend->SetApplication(app.get());
    const std::filesystem::path workspaceStatePath = GetEditorWorkspaceStatePath();

    FEditorShellWidgets shell = BuildEditorShell();
    auto workspaceController = std::make_shared<EditorWorkspaceController>(&BuildInitialDocumentRoot);
    shell.ShellHost->SetWorkspaceController(workspaceController);
    workspaceController->SetOnProjectStateChanged([weakApp = std::weak_ptr<ImApplication>(app), weakWorkspace = std::weak_ptr<EditorWorkspaceController>(workspaceController)]() {
        auto lockedApp = weakApp.lock();
        auto lockedWorkspace = weakWorkspace.lock();
        if (!lockedApp || !lockedWorkspace) {
            return;
        }

        RebuildTitleBarMenus(*lockedApp, lockedWorkspace);
    });
    workspaceController->SetOnExitRequested([weakBackend = std::weak_ptr<ImWin32DX11Backend>(backend)]() {
        if (auto lockedBackend = weakBackend.lock()) {
            lockedBackend->RequestClose();
        }
    });
    workspaceController->Bind(
        shell.ShellHost,
        shell.DocumentTabs,
        shell.ProjectView,
        shell.WidgetTreeView,
        shell.DetailsView,
        shell.OutputText);
    if (!workspaceController->LoadWorkspaceState(workspaceStatePath)) {
        workspaceController->SetProjectRoot(defaultWorkspaceDirectory);
    }

    app->SetApplicationTitle("ImWidgetV4 Editor");
    app->SetApplicationIcon(app->GetCoreIconBrush(ECoreIcon::Settings));
    app->ClearTitleBarActionButtons();
    app->AddTitleBarActionButton(FApplicationTitleBarActionButton {
        app->GetCoreIconBrush(ECoreIcon::Undo, FColor::FromBytes(210, 219, 232)),
        "Undo",
        [workspaceController]() {
            if (workspaceController) {
                workspaceController->Undo();
            }
        },
        [workspaceController]() {
            return workspaceController && workspaceController->GetActiveSession() && workspaceController->GetActiveSession()->CanUndo();
        },
        [workspaceController]() {
            return workspaceController && workspaceController->GetActiveSession() && workspaceController->GetActiveSession()->CanUndo();
        }
    });
    app->AddTitleBarActionButton(FApplicationTitleBarActionButton {
        app->GetCoreIconBrush(ECoreIcon::Redo, FColor::FromBytes(210, 219, 232)),
        "Redo",
        [workspaceController]() {
            if (workspaceController) {
                workspaceController->Redo();
            }
        },
        [workspaceController]() {
            return workspaceController && workspaceController->GetActiveSession() && workspaceController->GetActiveSession()->CanRedo();
        },
        [workspaceController]() {
            return workspaceController && workspaceController->GetActiveSession() && workspaceController->GetActiveSession()->CanRedo();
        }
    });
    RebuildTitleBarMenus(*app, workspaceController);
    app->SetRootWidget(shell.Root);

    backend->SetCloseRequestedHandler([weakApp = std::weak_ptr<ImApplication>(app), weakWorkspace = std::weak_ptr<EditorWorkspaceController>(workspaceController)]() {
        auto lockedApp = weakApp.lock();
        auto lockedWorkspace = weakWorkspace.lock();
        if (!lockedApp || !lockedWorkspace) {
            return true;
        }

        return lockedWorkspace->RequestApplicationClose(*lockedApp);
    });

    backend->Run();
    workspaceController->SaveWorkspaceState(workspaceStatePath);
    backend->Shutdown();
    return 0;
}
