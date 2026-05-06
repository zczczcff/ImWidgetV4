#include "editor/EditorSession.h"
#include "inspector/ReflectionDetailsView.h"
#include "palette/WidgetPaletteView.h"
#include "tree/DocumentTreeViewBinder.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include "../samples/DemoPaths.h"
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor;

namespace {

struct FEditorShellWidgets {
    std::shared_ptr<ImWidget> Root;
    std::shared_ptr<ImTabView> DocumentTabs;
    std::shared_ptr<ImScrollBox> DocumentHost;
    std::shared_ptr<ImDesignerSurface> DesignerSurface;
    std::shared_ptr<ImTextOutlineView> WidgetTreeView;
    std::shared_ptr<ReflectionDetailsView> DetailsView;
    std::shared_ptr<ImTextBlock> OutputText;
    int MainDocumentTabIndex = -1;
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

std::shared_ptr<ImWidget> BuildProjectViewPanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());

    ImTextOutlineItem* project = outline->AddRootItem("ImWidgetV4Editor");
    project->Expanded = true;

    ImTextOutlineItem* assets = outline->AddChildItem(project, "Assets");
    assets->Expanded = true;
    outline->AddChildItem(assets, "Main.ui.json");
    outline->AddChildItem(assets, "Preview.ui.json");

    ImTextOutlineItem* src = outline->AddChildItem(project, "Source");
    src->Expanded = true;
    outline->AddChildItem(src, "EditorDocument.cpp");
    outline->AddChildItem(src, "EditorSession.cpp");
    outline->AddChildItem(src, "WidgetSerializer.cpp");
    outline->AddChildItem(src, "main.cpp");

    outline->SetSelectedItem(project);
    return outline;
}

std::shared_ptr<ImWidget> BuildWidgetTreePanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());
    return outline;
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
    tabView->AddTab("Project", BuildProjectViewPanel());
    tabView->AddTab("Widget Tree", BuildWidgetTreePanel());
    return tabView;
}

std::shared_ptr<ImWidget> BuildInitialDocumentRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("DocumentRoot");
    canvas->SetDesiredSize(FVector2(640.0f, 420.0f));

    auto heading = std::make_shared<ImTextBlock>();
    heading->SetName("Heading");
    heading->SetText("Designer Surface");
    heading->SetFontSize(22.0f);
    heading->SetTextColor(FColor::White);
    canvas->AddChildAt(heading, FVector2(0.04f, 0.06f));

    auto body = std::make_shared<ImTextBlock>();
    body->SetName("Body");
    body->SetText("This central workspace is now owned by EditorDocument. Save and reopen will round-trip this widget tree through the editor serializer.");
    body->SetFontSize(16.0f);
    body->SetTextColor(FColor::FromBytes(196, 205, 218));
    canvas->AddChildAt(body, FVector2(0.04f, 0.18f), FVector2(0.62f, 0.26f));

    auto hint = std::make_shared<ImTextBlock>();
    hint->SetName("Hint");
    hint->SetText("Next step: replace this with a real designer surface.");
    hint->SetFontSize(15.0f);
    hint->SetTextColor(FColor::FromBytes(122, 188, 255));
    canvas->AddChildAt(hint, FVector2(0.04f, 0.52f));

    return canvas;
}

FEditorShellWidgets BuildEditorShell()
{
    FEditorShellWidgets shell;

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

    auto documentHost = std::make_shared<ImScrollBox>();
    FScrollBoxStyle scrollStyle = documentHost->GetStyle();
    scrollStyle.BackgroundColor = FColor::FromBytes(18, 23, 29);
    scrollStyle.BorderThickness = 0.0f;
    scrollStyle.CornerRadius = 0.0f;
    scrollStyle.Padding = FMargin(0.0f);
    documentHost->SetStyle(scrollStyle);

    auto designerSurface = std::make_shared<ImDesignerSurface>();
    const int mainDocumentTabIndex = documentTabs->AddTab("Main.ui", documentHost);
    documentHost->SetContent(designerSurface);
    documentTabs->AddTab("Preview", MakeSimplePanel(
        "Live Preview",
        "Runtime preview will be mounted here so the editor can inspect the same widget tree rendered by the core library."));
    documentTabs->AddTab("PropertiesSchema", MakeSimplePanel(
        "Schema Notes",
        "Reflection-backed metadata, property editors and drag/drop payload inspection can share this workspace during early editor development."));

    auto rightDock = std::make_shared<ImVerticalBox>();
    rightDock->SetSpacing(0.0f);
    auto detailsView = std::make_shared<ReflectionDetailsView>();
    rightDock->AddChildFill(detailsView, 1.0f, FMargin(0.0f));

    topWorkspace->AddPart(leftDock, 0.22f, 240.0f);
    topWorkspace->AddPart(documentTabs, 0.56f, 420.0f);
    topWorkspace->AddPart(rightDock, 0.22f, 260.0f);

    auto bottomDock = std::make_shared<ImVerticalBox>();
    bottomDock->SetSpacing(8.0f);
    bottomDock->AddChild(MakePanelTitle("Output"), FMargin(14.0f, 14.0f, 14.0f, 14.0f));
    auto outputText = MakePanelBody("Booting editor session...");
    bottomDock->AddChild(outputText, FMargin(14.0f, 0.0f, 14.0f, 14.0f));

    verticalShell->AddPart(topWorkspace, 0.78f, 360.0f);
    verticalShell->AddPart(bottomDock, 0.22f, 140.0f);

    shell.Root = verticalShell;
    shell.DocumentTabs = documentTabs;
    shell.DocumentHost = documentHost;
    shell.DesignerSurface = designerSurface;
    shell.WidgetTreeView = widgetTreeView;
    shell.DetailsView = detailsView;
    shell.OutputText = outputText;
    shell.MainDocumentTabIndex = mainDocumentTabIndex;
    return shell;
}

std::vector<FApplicationMenuItem> BuildFileMenuItems(
    ImApplication& app,
    const std::shared_ptr<EditorSession>& session)
{
    return {
        FApplicationMenuItem {"New", {}, {}, true, false, [&app, session]() {
            if (session) {
                session->NewDocument();
            }
        }},
        FApplicationMenuItem {"Open...", {}, {}, true, false, [&app, session]() {
            if (session) {
                session->OpenDocument(app);
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Save", {}, {}, true, false, [&app, session]() {
            if (session) {
                session->SaveDocument(app);
            }
        }},
        FApplicationMenuItem {"Save As...", {}, {}, true, false, [&app, session]() {
            if (session) {
                session->SaveDocumentAs(app);
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

std::vector<FApplicationMenuItem> BuildEditMenuItems(const std::shared_ptr<EditorSession>& session)
{
    return {
        FApplicationMenuItem {"Undo", {}, {}, true, false, [session]() {
            if (session) {
                session->Undo();
            }
        }},
        FApplicationMenuItem {"Redo", {}, {}, true, false, [session]() {
            if (session) {
                session->Redo();
            }
        }},
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        FApplicationMenuItem {"Coming Soon", {}, {}, false, false, {}}
    };
}

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
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

    FEditorShellWidgets shell = BuildEditorShell();
    auto session = std::make_shared<EditorSession>(&BuildInitialDocumentRoot);
    session->BindDocumentWidgets(
        shell.DocumentTabs,
        shell.MainDocumentTabIndex,
        shell.DocumentHost,
        shell.DesignerSurface,
        shell.WidgetTreeView,
        shell.DetailsView,
        shell.OutputText);

    app->SetApplicationTitle("ImWidgetV4 Editor");
    app->SetApplicationIcon(app->GetCoreIconBrush(ECoreIcon::Settings));
    app->AddTitleBarTabMenu("File", BuildFileMenuItems(*app, session));
    app->AddTitleBarTabMenu("Edit", BuildEditMenuItems(session));
    app->AddTitleBarTabMenu("View", BuildSimpleMenuItems("View"));
    app->AddTitleBarTabMenu(app->GetCoreIconBrush(ECoreIcon::Search), BuildSimpleMenuItems("Search"));
    app->SetRootWidget(shell.Root);

    backend->Run();
    backend->Shutdown();
    return 0;
}
