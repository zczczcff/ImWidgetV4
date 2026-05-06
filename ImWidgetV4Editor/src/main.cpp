#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/HorizontalSplitter.h>
#include <imwidgetv4/widgets/TabView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/widgets/VerticalSplitter.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include "../samples/DemoPaths.h"
#include <filesystem>
#include <memory>
#include <string>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

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
    body->SetWrapText(true);
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

std::shared_ptr<ImTextOutlineView> BuildControlPalettePanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());

    ImTextOutlineItem* layout = outline->AddRootItem("布局");
    layout->Expanded = true;
    outline->AddChildItem(layout, "HorizontalBox");
    outline->AddChildItem(layout, "VerticalBox");
    outline->AddChildItem(layout, "CanvasPanel");
    outline->AddChildItem(layout, "ScrollBox");
    outline->AddChildItem(layout, "TabView");

    ImTextOutlineItem* display = outline->AddRootItem("显示");
    display->Expanded = true;
    outline->AddChildItem(display, "TextBlock");
    outline->AddChildItem(display, "Image");
    outline->AddChildItem(display, "Border");
    outline->AddChildItem(display, "ExpandableBox");

    ImTextOutlineItem* input = outline->AddRootItem("输入");
    input->Expanded = true;
    outline->AddChildItem(input, "Button");
    outline->AddChildItem(input, "Switch");
    outline->AddChildItem(input, "CheckBox");
    outline->AddChildItem(input, "Slider");
    outline->AddChildItem(input, "EditableText");

    outline->SetSelectedItem(layout);
    return outline;
}

std::shared_ptr<ImTextOutlineView> BuildProjectViewPanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());

    ImTextOutlineItem* project = outline->AddRootItem("ImWidgetV4Editor");
    project->Expanded = true;

    ImTextOutlineItem* assets = outline->AddChildItem(project, "Assets");
    assets->Expanded = true;
    outline->AddChildItem(assets, "Main.ui");
    outline->AddChildItem(assets, "Preview.ui");

    ImTextOutlineItem* src = outline->AddChildItem(project, "Source");
    src->Expanded = true;
    outline->AddChildItem(src, "EditorShell.cpp");
    outline->AddChildItem(src, "PaletteView.cpp");
    outline->AddChildItem(src, "PropertyInspector.cpp");

    ImTextOutlineItem* generated = outline->AddChildItem(project, "Generated");
    generated->Expanded = true;
    outline->AddChildItem(generated, "ReflectionCache.json");

    outline->SetSelectedItem(project);
    return outline;
}

std::shared_ptr<ImTextOutlineView> BuildWidgetTreePanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeDockOutlineStyle());

    ImTextOutlineItem* root = outline->AddRootItem("RootWindow");
    root->Expanded = true;

    ImTextOutlineItem* shell = outline->AddChildItem(root, "EditorShell");
    shell->Expanded = true;

    ImTextOutlineItem* leftDock = outline->AddChildItem(shell, "HierarchyDock");
    leftDock->Expanded = true;
    outline->AddChildItem(leftDock, "WidgetTree");
    outline->AddChildItem(leftDock, "Palette");

    ImTextOutlineItem* centerDock = outline->AddChildItem(shell, "DocumentDock");
    centerDock->Expanded = true;
    outline->AddChildItem(centerDock, "Main.ui");
    outline->AddChildItem(centerDock, "Preview");

    ImTextOutlineItem* rightDock = outline->AddChildItem(shell, "DetailsDock");
    rightDock->Expanded = true;
    outline->AddChildItem(rightDock, "Selection");
    outline->AddChildItem(rightDock, "Layout");
    outline->AddChildItem(rightDock, "Appearance");

    outline->SetSelectedItem(shell);
    return outline;
}

std::shared_ptr<ImTabView> BuildLeftDockTabs()
{
    auto tabView = std::make_shared<ImTabView>();
    tabView->SetSupportsKeyboardFocus(true);
    tabView->SetTabStripPlacement(ETabStripPlacement::Bottom);

    FTabViewStyle style = tabView->GetStyle();
    style.Padding = FMargin(0.0f);
    style.TabPadding = FMargin(8.0f, 3.0f, 4.0f, 3.0f);
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

    tabView->AddTab("控件列表", BuildControlPalettePanel());
    tabView->AddTab("项目视图", BuildProjectViewPanel());
    tabView->AddTab("控件树视图", BuildWidgetTreePanel());
    return tabView;
}

std::shared_ptr<ImWidget> BuildDesignerSurface()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetDesiredSize(FVector2(640.0f, 420.0f));

    auto heading = std::make_shared<ImTextBlock>();
    heading->SetText("Designer Surface");
    heading->SetFontSize(22.0f);
    heading->SetTextColor(FColor::White);
    canvas->AddChildAt(heading, FVector2(0.04f, 0.06f));

    auto body = std::make_shared<ImTextBlock>();
    body->SetText("This central workspace is reserved for the upcoming visual designer. The surrounding docks already use retained-mode widgets and splitters, so the editor shell can grow from here.");
    body->SetWrapText(true);
    body->SetFontSize(16.0f);
    body->SetTextColor(FColor::FromBytes(196, 205, 218));
    canvas->AddChildAt(body, FVector2(0.04f, 0.18f), FVector2(0.56f, 0.26f));

    auto hint = std::make_shared<ImTextBlock>();
    hint->SetText("Drag palette items here next.");
    hint->SetFontSize(15.0f);
    hint->SetTextColor(FColor::FromBytes(122, 188, 255));
    canvas->AddChildAt(hint, FVector2(0.04f, 0.52f));

    return canvas;
}

std::shared_ptr<ImTabView> BuildDocumentTabs()
{
    auto tabView = std::make_shared<ImTabView>();
    tabView->SetSupportsKeyboardFocus(true);

    FTabViewStyle style = tabView->GetStyle();
    style.Padding = FMargin(0.0f);
    style.TabHeight = 36.0f;
    style.TabMinWidth = 150.0f;
    style.TabSpacing = 0.0f;
    style.BorderThickness = 0.0f;
    style.CornerRadius = 0.0f;
    style.TabStripBackgroundColor = FColor::FromBytes(27, 33, 41);
    style.BackgroundColor = FColor::FromBytes(18, 23, 29);
    style.ActiveTabColor = FColor::FromBytes(63, 90, 128);
    tabView->SetStyle(style);

    tabView->AddTab("Main.ui", BuildDesignerSurface());
    tabView->AddTab("Preview", MakeSimplePanel(
        "Live Preview",
        "Runtime preview will be mounted here so the editor can inspect the same widget tree rendered by the core library."));
    tabView->AddTab("PropertiesSchema", MakeSimplePanel(
        "Schema Notes",
        "Reflection-backed metadata, property editors and drag/drop payload inspection can share this workspace during early editor development."));

    return tabView;
}

std::shared_ptr<ImWidget> BuildEditorRoot()
{
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

    auto centerDock = BuildDocumentTabs();

    auto rightDock = std::make_shared<ImVerticalBox>();
    rightDock->SetSpacing(10.0f);
    rightDock->AddChild(MakePanelTitle("Details"), FMargin(14.0f, 14.0f, 14.0f, 14.0f));
    rightDock->AddChild(MakePanelBody(
        "The right dock is reserved for reflection-driven property editing. Selection summaries, category groups and custom editors will land here next."), FMargin(14.0f, 0.0f, 14.0f, 6.0f));
    rightDock->AddChild(MakeSimplePanel(
        "Selection",
        "No widget selected.\nChoose a node from the hierarchy or the designer surface to populate this panel."), FMargin(10.0f));

    topWorkspace->AddPart(leftDock, 0.22f, 240.0f);
    topWorkspace->AddPart(centerDock, 0.56f, 420.0f);
    topWorkspace->AddPart(rightDock, 0.22f, 260.0f);

    auto bottomDock = std::make_shared<ImVerticalBox>();
    bottomDock->SetSpacing(8.0f);
    bottomDock->AddChild(MakePanelTitle("Output"), FMargin(14.0f, 14.0f, 14.0f, 14.0f));
    bottomDock->AddChild(MakePanelBody(
        "[Info] Editor shell booted with custom host chrome.\n[Info] Core library and editor workspace are now split.\n[Next] Palette, designer interactions and property inspector editing will build on this layout."), FMargin(14.0f, 0.0f, 14.0f, 14.0f));

    verticalShell->AddPart(topWorkspace, 0.78f, 360.0f);
    verticalShell->AddPart(bottomDock, 0.22f, 140.0f);
    return verticalShell;
}

std::vector<FApplicationMenuItem> BuildMenuItems(const std::string& menuName)
{
    std::vector<FApplicationMenuItem> items;
    items.push_back(FApplicationMenuItem {menuName + " Action", FImageBrush(), {}, true, false, []() {}});
    items.push_back(FApplicationMenuItem {"", FImageBrush(), {}, true, true, {}});
    items.push_back(FApplicationMenuItem {"Coming Soon", FImageBrush(), {}, false, false, {}});
    return items;
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
    app->SetApplicationTitle("ImWidgetV4 Editor");
    app->SetApplicationIcon(app->GetCoreIconBrush(ECoreIcon::Settings));
    app->AddTitleBarTabMenu("File", BuildMenuItems("File"));
    app->AddTitleBarTabMenu("Edit", BuildMenuItems("Edit"));
    app->AddTitleBarTabMenu("View", BuildMenuItems("View"));
    app->AddTitleBarTabMenu(app->GetCoreIconBrush(ECoreIcon::Search), BuildMenuItems("Search"));
    app->SetRootWidget(BuildEditorRoot());
    backend->Run();
    backend->Shutdown();
    return 0;
}
