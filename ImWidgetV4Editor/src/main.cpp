#include "editor/EditorSession.h"
#include "editor/EditorShellHost.h"
#include "editor/EditorLocalization.h"
#include "editor/EditorPaths.h"
#include "editor/EditorWorkspaceController.h"
#include "inspector/ReflectionDetailsView.h"
#include "inspector/PropertyEditorWidgets.h"
#include "palette/WidgetPaletteView.h"
#include "toolchains/EnvironmentProbe.h"
#include "tree/DocumentTreeViewBinder.h"

#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/ApplicationBackend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/EditableText.h>
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
#include <algorithm>
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
    std::shared_ptr<ImComboBox> BuildProfileComboBox;
    std::shared_ptr<ImComboBox> BuildWindowsGeneratorComboBox;
    std::shared_ptr<ImComboBox> BuildAndroidAbiComboBox;
    std::shared_ptr<ImComboBox> BuildAndroidApiComboBox;
    std::shared_ptr<ImComboBox> BuildAndroidStlComboBox;
    std::shared_ptr<ImEditableText> BuildAndroidSdkRootEditor;
    std::shared_ptr<ImEditableText> BuildAndroidNdkRootEditor;
    std::shared_ptr<ImButton> BuildConfigureButton;
    std::shared_ptr<ImButton> BuildRunButton;
    std::shared_ptr<ImButton> BuildCleanButton;
    std::shared_ptr<ImButton> BuildRebuildButton;
    std::shared_ptr<ImButton> BuildApplyProfileButton;
    std::shared_ptr<ImButton> BuildReprobeButton;
    std::shared_ptr<ImButton> BuildSettingsButton;
    std::shared_ptr<ImButton> BuildRevealButton;
    std::shared_ptr<ImButton> BuildAndroidSdkBrowseButton;
    std::shared_ptr<ImButton> BuildAndroidSdkClearButton;
    std::shared_ptr<ImButton> BuildAndroidNdkBrowseButton;
    std::shared_ptr<ImButton> BuildAndroidNdkClearButton;
    std::shared_ptr<ImVerticalBox> BuildWindowsSettingsGroup;
    std::shared_ptr<ImVerticalBox> BuildAndroidSettingsGroup;
    std::shared_ptr<ImTextList> BuildOverviewText;
    std::shared_ptr<ReflectionDetailsView> DetailsView;
    std::shared_ptr<ImTextList> OutputText;
    std::shared_ptr<ImImage> TitleBarIcon;
    std::shared_ptr<ImTextBlock> TitleBarText;
    std::shared_ptr<ImTextBlock> TitleBarProfileStatusText;
    std::shared_ptr<ImButton> UndoButton;
    std::shared_ptr<ImButton> RedoButton;
    bool bLastCanUndo = false;
    bool bLastCanRedo = false;
    std::string LastTitleBarProfileStatusText;
    FColor LastTitleBarProfileStatusColor = FColor::Transparent;
    std::string BuildDraftProfileName;
    std::string BuildDraftWindowsGenerator;
    std::string BuildDraftAndroidAbi;
    std::string BuildDraftAndroidApi;
    std::string BuildDraftAndroidStl;
    std::string BuildDraftAndroidSdkRoot;
    std::string BuildDraftAndroidNdkRoot;
    bool bBuildProfileDraftDirty = false;
    bool bBuildProfileDraftSyncing = false;
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

std::shared_ptr<FCompactTitleBarButton> MakeTitleBarTextButton(const FText& text)
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

std::shared_ptr<FCompactTitleBarButton> MakeTitleBarIconButton(const FImageBrush& brush, const FText& tooltip)
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

std::shared_ptr<ImTextList> MakePanelTextList(const std::vector<FText>& items)
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
        EditorText("Build.NoProjectLoaded", "No project loaded."),
        FText::FromString(""),
        EditorText("Build.OverviewHint", "Build/Toolchain overview will appear here.")
    });
}

std::vector<std::string> GetBuildDockWindowsGeneratorOptions()
{
    return {
        "Default",
        "Ninja",
        "Visual Studio 17 2022"
    };
}

std::string ResolveBuildDockWindowsGeneratorValue(const std::string& generator)
{
    return generator.empty() ? std::string("Default") : generator;
}

std::vector<std::string> GetBuildDockAndroidAbiOptions()
{
    return {"arm64-v8a", "armeabi-v7a", "x86_64", "x86"};
}

std::vector<std::string> GetBuildDockAndroidApiOptions()
{
    return {"21", "23", "24", "26", "28", "29", "30", "31", "33", "34"};
}

std::vector<std::string> GetBuildDockAndroidStlOptions()
{
    return {"c++_shared", "c++_static"};
}

std::shared_ptr<ImHorizontalBox> MakeBuildDockPathEditorRow(
    const std::shared_ptr<ImEditableText>& editor,
    const std::shared_ptr<ImButton>& browseButton,
    const std::shared_ptr<ImButton>& clearButton)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(6.0f);
    row->AddChildFill(editor, 1.0f, FMargin(0.0f));
    row->AddChild(browseButton, FMargin(0.0f));
    row->AddChild(clearButton, FMargin(0.0f));
    return row;
}

int FindStringOptionIndex(const std::vector<std::string>& options, const std::string& value)
{
    for (int index = 0; index < static_cast<int>(options.size()); ++index) {
        if (options[static_cast<std::size_t>(index)] == value) {
            return index;
        }
    }

    return -1;
}

void SyncBuildDockDraftFromProfile(FEditorShellWidgets& shell, const FEditorBuildProfile* profile)
{
    shell.BuildDraftProfileName = profile ? profile->Name : std::string();
    shell.BuildDraftWindowsGenerator = profile ? profile->Generator : std::string();
    shell.BuildDraftAndroidAbi = profile ? profile->AndroidSettings.Abi : std::string();
    shell.BuildDraftAndroidApi = profile ? std::to_string(profile->AndroidSettings.ApiLevel) : std::string();
    shell.BuildDraftAndroidStl = profile ? profile->AndroidSettings.Stl : std::string();
    shell.BuildDraftAndroidSdkRoot = profile ? profile->AndroidSettings.SdkRootOverride.string() : std::string();
    shell.BuildDraftAndroidNdkRoot = profile ? profile->AndroidSettings.NdkRootOverride.string() : std::string();
    shell.bBuildProfileDraftDirty = false;
}

void ApplyBuildDockDraftToWidgets(FEditorShellWidgets& shell, const FEditorBuildProfile* profile)
{
    shell.bBuildProfileDraftSyncing = true;

    if (shell.BuildWindowsGeneratorComboBox) {
        const std::vector<std::string> generatorOptions = GetBuildDockWindowsGeneratorOptions();
        shell.BuildWindowsGeneratorComboBox->SetItems(generatorOptions);
        const int generatorIndex =
            FindStringOptionIndex(generatorOptions, ResolveBuildDockWindowsGeneratorValue(shell.BuildDraftWindowsGenerator));
        if (generatorIndex >= 0) {
            shell.BuildWindowsGeneratorComboBox->SetSelectedIndex(generatorIndex);
        } else {
            shell.BuildWindowsGeneratorComboBox->ClearSelection();
        }
    }

    if (shell.BuildAndroidAbiComboBox) {
        const std::vector<std::string> abiOptions = GetBuildDockAndroidAbiOptions();
        shell.BuildAndroidAbiComboBox->SetItems(abiOptions);
        const int abiIndex = FindStringOptionIndex(abiOptions, shell.BuildDraftAndroidAbi);
        if (abiIndex >= 0) {
            shell.BuildAndroidAbiComboBox->SetSelectedIndex(abiIndex);
        } else {
            shell.BuildAndroidAbiComboBox->ClearSelection();
        }
    }

    if (shell.BuildAndroidApiComboBox) {
        std::vector<std::string> apiOptions = GetBuildDockAndroidApiOptions();
        if (!shell.BuildDraftAndroidApi.empty() &&
            FindStringOptionIndex(apiOptions, shell.BuildDraftAndroidApi) < 0) {
            apiOptions.push_back(shell.BuildDraftAndroidApi);
            std::sort(apiOptions.begin(), apiOptions.end(), [](const std::string& left, const std::string& right) {
                return std::stoi(left) < std::stoi(right);
            });
        }

        shell.BuildAndroidApiComboBox->SetItems(apiOptions);
        const int apiIndex = FindStringOptionIndex(apiOptions, shell.BuildDraftAndroidApi);
        if (apiIndex >= 0) {
            shell.BuildAndroidApiComboBox->SetSelectedIndex(apiIndex);
        } else {
            shell.BuildAndroidApiComboBox->ClearSelection();
        }
    }

    if (shell.BuildAndroidStlComboBox) {
        const std::vector<std::string> stlOptions = GetBuildDockAndroidStlOptions();
        shell.BuildAndroidStlComboBox->SetItems(stlOptions);
        const int stlIndex = FindStringOptionIndex(stlOptions, shell.BuildDraftAndroidStl);
        if (stlIndex >= 0) {
            shell.BuildAndroidStlComboBox->SetSelectedIndex(stlIndex);
        } else {
            shell.BuildAndroidStlComboBox->ClearSelection();
        }
    }

    if (shell.BuildAndroidSdkRootEditor) {
        shell.BuildAndroidSdkRootEditor->SetText(shell.BuildDraftAndroidSdkRoot);
    }
    if (shell.BuildAndroidNdkRootEditor) {
        shell.BuildAndroidNdkRootEditor->SetText(shell.BuildDraftAndroidNdkRoot);
    }

    const bool bShowWindowsSettings =
        profile != nullptr && profile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop;
    const bool bShowAndroidSettings =
        profile != nullptr && profile->TargetPlatform == EEditorTargetPlatform::Android;
    if (shell.BuildWindowsSettingsGroup) {
        shell.BuildWindowsSettingsGroup->SetVisible(bShowWindowsSettings);
    }
    if (shell.BuildAndroidSettingsGroup) {
        shell.BuildAndroidSettingsGroup->SetVisible(bShowAndroidSettings);
    }

    shell.bBuildProfileDraftSyncing = false;
}

std::shared_ptr<ImWidget> BuildBuildDockPanel(FEditorShellWidgets& shell)
{
    using namespace ImWidgetV4Editor::PropertyEditorWidgets;

    auto profileComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*profileComboBox);
    profileComboBox->SetItems(std::vector<std::string>{});

    auto windowsGeneratorComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*windowsGeneratorComboBox);
    windowsGeneratorComboBox->SetItems(GetBuildDockWindowsGeneratorOptions());

    auto androidAbiComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*androidAbiComboBox);
    androidAbiComboBox->SetItems(GetBuildDockAndroidAbiOptions());

    auto androidApiComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*androidApiComboBox);
    androidApiComboBox->SetItems(GetBuildDockAndroidApiOptions());

    auto androidStlComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*androidStlComboBox);
    androidStlComboBox->SetItems(GetBuildDockAndroidStlOptions());

    auto androidSdkRootEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*androidSdkRootEditor, false);
    androidSdkRootEditor->SetHintText(EditorText("Build.OverrideAndroidSdkRoot", "Override Android SDK root"));

    auto androidSdkBrowseButton = std::make_shared<ImButton>();
    androidSdkBrowseButton->SetText(EditorText("Build.Browse", "Browse"));

    auto androidSdkClearButton = std::make_shared<ImButton>();
    androidSdkClearButton->SetText(EditorText("Build.Clear", "Clear"));

    auto androidNdkRootEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*androidNdkRootEditor, false);
    androidNdkRootEditor->SetHintText(EditorText("Build.OverrideAndroidNdkRoot", "Override Android NDK root"));

    auto androidNdkBrowseButton = std::make_shared<ImButton>();
    androidNdkBrowseButton->SetText(EditorText("Build.Browse", "Browse"));

    auto androidNdkClearButton = std::make_shared<ImButton>();
    androidNdkClearButton->SetText(EditorText("Build.Clear", "Clear"));

    auto applyProfileButton = std::make_shared<ImButton>();
    applyProfileButton->SetText(EditorText("Build.Apply", "Apply"));

    auto reprobeButton = std::make_shared<ImButton>();
    reprobeButton->SetText(EditorText("Build.ReProbe", "Re-Probe"));

    auto configureButton = std::make_shared<ImButton>();
    configureButton->SetText(EditorText("Build.Configure", "Configure"));

    auto buildButton = std::make_shared<ImButton>();
    buildButton->SetText(EditorText("Build.Build", "Build"));

    auto cleanButton = std::make_shared<ImButton>();
    cleanButton->SetText(EditorText("Build.Clean", "Clean"));

    auto rebuildButton = std::make_shared<ImButton>();
    rebuildButton->SetText(EditorText("Build.Rebuild", "Rebuild"));

    auto settingsButton = std::make_shared<ImButton>();
    settingsButton->SetText(EditorText("Build.Settings", "Settings"));

    auto revealButton = std::make_shared<ImButton>();
    revealButton->SetText(EditorText("Build.Reveal", "Reveal"));

    auto windowsSettingsGroup = std::make_shared<ImVerticalBox>();
    windowsSettingsGroup->SetSpacing(6.0f);
    windowsSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow(EditorText("Build.Generator", "Generator").Resolve(), windowsGeneratorComboBox),
        FMargin(8.0f, 0.0f, 8.0f, 0.0f));

    auto androidSettingsGroup = std::make_shared<ImVerticalBox>();
    androidSettingsGroup->SetSpacing(6.0f);
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow("ABI", androidAbiComboBox),
        FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow("API", androidApiComboBox),
        FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow("STL", androidStlComboBox),
        FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow(
            EditorText("Build.AndroidSdkRoot", "Android SDK Root").Resolve(),
            MakeBuildDockPathEditorRow(androidSdkRootEditor, androidSdkBrowseButton, androidSdkClearButton)),
        FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow(
            EditorText("Build.AndroidNdkRoot", "Android NDK Root").Resolve(),
            MakeBuildDockPathEditorRow(androidNdkRootEditor, androidNdkBrowseButton, androidNdkClearButton)),
        FMargin(8.0f, 0.0f, 8.0f, 0.0f));

    auto profileActionRow = std::make_shared<ImHorizontalBox>();
    profileActionRow->SetSpacing(6.0f);
    profileActionRow->AddChildFill(applyProfileButton, 1.0f, FMargin(0.0f));
    profileActionRow->AddChildFill(reprobeButton, 1.0f, FMargin(0.0f));
    profileActionRow->AddChildFill(settingsButton, 1.0f, FMargin(0.0f));

    auto buttonRow = std::make_shared<ImHorizontalBox>();
    buttonRow->SetSpacing(6.0f);
    buttonRow->AddChildFill(configureButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(buildButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(cleanButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(rebuildButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(revealButton, 1.0f, FMargin(0.0f));

    auto overviewText = BuildBuildOverviewPanel();

    auto panel = std::make_shared<ImVerticalBox>();
    panel->SetSpacing(8.0f);
    panel->AddChild(MakePanelTitle(EditorText("Build.PanelTitle", "Build").Resolve()), FMargin(10.0f, 10.0f, 10.0f, 0.0f));
    panel->AddChild(MakePanelBody(EditorText("Build.PanelBody", "Toolchain readiness, active profile, and recent build output.").Resolve()), FMargin(10.0f, 0.0f, 10.0f, 0.0f));
    panel->AddChild(MakeInspectorVerticalPropertyRow(EditorText("Build.ActiveProfile", "Active Profile").Resolve(), profileComboBox), FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    panel->AddChild(windowsSettingsGroup, FMargin(0.0f));
    panel->AddChild(androidSettingsGroup, FMargin(0.0f));
    panel->AddChild(profileActionRow, FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    panel->AddChild(buttonRow, FMargin(8.0f, 0.0f, 8.0f, 0.0f));
    panel->AddChildFill(overviewText, 1.0f, FMargin(0.0f));

    shell.BuildProfileComboBox = profileComboBox;
    shell.BuildWindowsGeneratorComboBox = windowsGeneratorComboBox;
    shell.BuildAndroidAbiComboBox = androidAbiComboBox;
    shell.BuildAndroidApiComboBox = androidApiComboBox;
    shell.BuildAndroidStlComboBox = androidStlComboBox;
    shell.BuildAndroidSdkRootEditor = androidSdkRootEditor;
    shell.BuildAndroidNdkRootEditor = androidNdkRootEditor;
    shell.BuildConfigureButton = configureButton;
    shell.BuildRunButton = buildButton;
    shell.BuildCleanButton = cleanButton;
    shell.BuildRebuildButton = rebuildButton;
    shell.BuildApplyProfileButton = applyProfileButton;
    shell.BuildReprobeButton = reprobeButton;
    shell.BuildSettingsButton = settingsButton;
    shell.BuildRevealButton = revealButton;
    shell.BuildAndroidSdkBrowseButton = androidSdkBrowseButton;
    shell.BuildAndroidSdkClearButton = androidSdkClearButton;
    shell.BuildAndroidNdkBrowseButton = androidNdkBrowseButton;
    shell.BuildAndroidNdkClearButton = androidNdkClearButton;
    shell.BuildWindowsSettingsGroup = windowsSettingsGroup;
    shell.BuildAndroidSettingsGroup = androidSettingsGroup;
    shell.BuildOverviewText = overviewText;
    return panel;
}

std::shared_ptr<ImWidget> BuildInitialDocumentRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("RootCanvas");
    canvas->SetDesiredSize(FVector2(1280.0f, 720.0f));

    auto title = std::make_shared<ImTextBlock>();
    title->SetName("TitleText");
    title->SetText(EditorText("App.Title", "ImWidgetV4 Editor"));
    title->SetFontSize(32.0f);
    title->SetWrapText(false);
    title->SetTextColor(FColor::FromBytes(235, 240, 248));
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(title, FVector2(0.08f, 0.08f))) {
        slot->SetAutoSize(true);
    }

    auto hint = std::make_shared<ImTextBlock>();
    hint->SetName("HintText");
    hint->SetText(EditorText("App.InitialHint", "Drag widgets from the left palette into the designer surface."));
    hint->SetFontSize(18.0f);
    hint->SetWrapText(false);
    hint->SetTextColor(FColor::FromBytes(162, 175, 191));
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(hint, FVector2(0.08f, 0.16f))) {
        slot->SetAutoSize(true);
    }

    auto button = std::make_shared<ImButton>();
    button->SetName("PrimaryButton");
    button->SetText(EditorText("App.Action", "Action"));
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(button, FVector2(0.08f, 0.28f))) {
        slot->SetAutoSize(true);
    }

    return canvas;
}

std::shared_ptr<ImTabView> BuildLeftDockTabs(FEditorShellWidgets& shell)
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

    tabView->AddTab(EditorText("Dock.Controls", "Controls"), BuildControlPalettePanel());
    auto projectView = BuildProjectViewPanel();
    tabView->AddTab(EditorText("Dock.Project", "Project"), projectView);
    tabView->AddTab(EditorText("Dock.Build", "Build"), BuildBuildDockPanel(shell));
    tabView->AddTab(EditorText("Dock.WidgetTree", "Widget Tree"), BuildWidgetTreePanel());
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
    titleText->SetText(EditorText("App.Title", "ImWidgetV4 Editor"));
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

    auto undoButton = MakeTitleBarIconButton(FImageBrush(), EditorText("TitleBar.Undo", "Undo"));
    auto redoButton = MakeTitleBarIconButton(FImageBrush(), EditorText("TitleBar.Redo", "Redo"));

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

    auto leftDock = BuildLeftDockTabs(shell);
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
    auto outputText = MakePanelTextList({EditorText("Output.Booting", "Booting editor session...")});
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
        FApplicationMenuItem {"Clean Active Profile", {}, {}, bHasProject && !bBuildRunning, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->CleanProject();
            }
        }},
        FApplicationMenuItem {"Rebuild Active Profile", {}, {}, bHasProject && !bBuildRunning, false, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RebuildProject();
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
                FEnvironmentProbeReport probeReport;
                const bool bHasProbeReport =
                    workspaceController->TryGetBuildProfileProbeReport(profile.Name, probeReport);
                const bool bProbeRefreshing =
                    workspaceController->IsBuildProfileProbeRefreshing(profile.Name);
                const std::string readinessLabel = bHasProbeReport
                    ? (probeReport.bReady ? "Ready" : "Needs Setup")
                    : (bProbeRefreshing ? "Refreshing" : "Unknown");
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
                        "Clean This Profile",
                        {},
                        {},
                        !bBuildRunning,
                        false,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->CleanProject(profileName);
                            }
                        }
                    },
                    FApplicationMenuItem {
                        "Rebuild This Profile",
                        {},
                        {},
                        !bBuildRunning,
                        false,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->RebuildProject(profileName);
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

    auto fileButton = MakeTitleBarTextButton(EditorText("TitleBar.File", "File"));
    BindPopupMenuButton(app, fileButton, [&app, workspaceController]() {
        return BuildFileMenuItems(app, workspaceController);
    });
    shell.TitleBar->AddLeadingItem(fileButton);

    auto editButton = MakeTitleBarTextButton(EditorText("TitleBar.Edit", "Edit"));
    BindPopupMenuButton(app, editButton, [workspaceController]() {
        return BuildEditMenuItems(workspaceController);
    });
    shell.TitleBar->AddLeadingItem(editButton);

    auto projectButton = MakeTitleBarTextButton(EditorText("TitleBar.Project", "Project"));
    BindPopupMenuButton(app, projectButton, [&app, workspaceController]() {
        return BuildProjectMenuItems(app, workspaceController);
    });
    shell.TitleBar->AddLeadingItem(projectButton);

    auto buildButton = MakeTitleBarTextButton(EditorText("TitleBar.Build", "Build"));
    BindPopupMenuButton(app, buildButton, [workspaceController]() {
        return BuildBuildMenuItems(workspaceController);
    });
    shell.TitleBar->AddLeadingItem(buildButton);

    auto viewButton = MakeTitleBarTextButton(EditorText("TitleBar.View", "View"));
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

    auto searchButton = MakeTitleBarIconButton(
        app.GetCoreIconBrush(ECoreIcon::Search, FColor::FromBytes(210, 219, 232)),
        EditorText("TitleBar.Search", "Search"));
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

    if (shell.UndoButton && shell.bLastCanUndo != bCanUndo) {
        shell.bLastCanUndo = bCanUndo;
        shell.UndoButton->SetDisabled(!bCanUndo);
        shell.UndoButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Undo, bCanUndo ? FColor::FromBytes(235, 242, 250) : FColor::FromBytes(132, 140, 150)),
            16.0f));
    }

    if (shell.RedoButton && shell.bLastCanRedo != bCanRedo) {
        shell.bLastCanRedo = bCanRedo;
        shell.RedoButton->SetDisabled(!bCanRedo);
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
                } else {
                    FEnvironmentProbeReport probeReport;
                    if (workspaceController->TryGetBuildProfileProbeReport(activeProfileName, probeReport)) {
                        if (probeReport.bReady) {
                            statusText += " | Ready";
                            statusColor = FColor::FromBytes(125, 204, 138);
                        } else {
                            statusText += " | Needs Setup";
                            statusColor = FColor::FromBytes(230, 184, 104);
                        }
                    } else if (workspaceController->IsBuildProfileProbeRefreshing(activeProfileName)) {
                        statusText += " | Checking...";
                        statusColor = FColor::FromBytes(103, 177, 255);
                    }
                }
            }
        }

        if (shell.LastTitleBarProfileStatusText != statusText) {
            shell.LastTitleBarProfileStatusText = statusText;
            shell.TitleBarProfileStatusText->SetText(statusText);
        }
        if (shell.LastTitleBarProfileStatusColor.R != statusColor.R ||
            shell.LastTitleBarProfileStatusColor.G != statusColor.G ||
            shell.LastTitleBarProfileStatusColor.B != statusColor.B ||
            shell.LastTitleBarProfileStatusColor.A != statusColor.A) {
            shell.LastTitleBarProfileStatusColor = statusColor;
            shell.TitleBarProfileStatusText->SetTextColor(statusColor);
        }
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
    const bool bProbeRefreshing = workspaceController->IsActiveBuildProfileProbeRefreshing();
    const std::shared_ptr<EditorProject> project = workspaceController->GetProject();
    const FEditorBuildProfile* activeProfile =
        (project && !activeProfileName.empty()) ? project->FindBuildProfile(activeProfileName) : nullptr;
    lines.push_back("Active Profile: " + (activeProfileName.empty() ? std::string("None") : activeProfileName));
    lines.push_back("Build Status: " + (workspaceController->IsBuildTaskRunning()
        ? workspaceController->GetBuildTaskStatusText()
        : std::string("Idle")));
    if (bProbeRefreshing) {
        lines.push_back("Probe Ready: Refreshing...");
    } else if (!probeReport.Items.empty()) {
        lines.push_back("Probe Ready: " + std::string(probeReport.bReady ? "Yes" : "No"));
    } else {
        lines.push_back("Probe Ready: Unknown");
    }
    lines.push_back("");

    if (activeProfile != nullptr) {
        lines.push_back("Profile Details:");
        lines.push_back("  Target: " + GetTargetPlatformDisplayName(activeProfile->TargetPlatform));
        lines.push_back("  Configuration: " + activeProfile->Configuration);
        lines.push_back("  Generator: " + (activeProfile->Generator.empty() ? std::string("Default") : activeProfile->Generator));
        lines.push_back("  Build Directory: " + ResolveBuildDirectoryPath(project->GetProjectRoot(), *activeProfile).string());

        if (activeProfile->TargetPlatform == EEditorTargetPlatform::Android) {
            lines.push_back("  Android ABI: " + activeProfile->AndroidSettings.Abi);
            lines.push_back("  Android API: " + std::to_string(activeProfile->AndroidSettings.ApiLevel));
            lines.push_back("  Android STL: " + activeProfile->AndroidSettings.Stl);
            if (!activeProfile->AndroidSettings.SdkRootOverride.empty()) {
                lines.push_back("  SDK Override: " + activeProfile->AndroidSettings.SdkRootOverride.string());
            }
            if (!activeProfile->AndroidSettings.NdkRootOverride.empty()) {
                lines.push_back("  NDK Override: " + activeProfile->AndroidSettings.NdkRootOverride.string());
            }
        }

        lines.push_back("");
    }

    if (bProbeRefreshing) {
        lines.push_back("Environment Probe:");
        lines.push_back("  Refreshing toolchain status...");
    } else if (probeReport.Items.empty()) {
        lines.push_back("No probe data available.");
    } else {
        std::vector<std::string> missingItems;
        for (const FEnvironmentProbeItem& item : probeReport.Items) {
            if (item.Status == EEnvironmentProbeStatus::Missing) {
                missingItems.push_back(item.Label);
            }
        }

        if (!missingItems.empty()) {
            lines.push_back("Setup Needed:");
            for (const std::string& missingItem : missingItems) {
                lines.push_back("  Missing " + missingItem);
            }
            lines.push_back("  Use Settings to update this profile and re-probe.");
            lines.push_back("");
        }

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

void UpdateBuildDockActions(
    FEditorShellWidgets& shell,
    ImApplication& app,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    const bool bHasProject = workspaceController && workspaceController->GetProject();
    const bool bBuildRunning = workspaceController && workspaceController->IsBuildTaskRunning();
    const std::string activeProfileName =
        workspaceController ? workspaceController->GetActiveBuildProfileName() : std::string();
    const std::shared_ptr<EditorProject> project =
        workspaceController ? workspaceController->GetProject() : nullptr;
    const FEditorBuildProfile* activeProfile =
        (project && !activeProfileName.empty()) ? project->FindBuildProfile(activeProfileName) : nullptr;

    if (shell.BuildProfileComboBox) {
        const std::vector<std::string> profileNames =
            workspaceController ? workspaceController->GetBuildProfileNames() : std::vector<std::string>();
        shell.BuildProfileComboBox->SetItems(profileNames);

        int selectedIndex = -1;
        for (int index = 0; index < static_cast<int>(profileNames.size()); ++index) {
            if (profileNames[static_cast<std::size_t>(index)] == activeProfileName) {
                selectedIndex = index;
                break;
            }
        }

        if (selectedIndex >= 0) {
            shell.BuildProfileComboBox->SetSelectedIndex(selectedIndex);
        } else {
            shell.BuildProfileComboBox->ClearSelection();
        }
        shell.BuildProfileComboBox->SetDisabled(!bHasProject || bBuildRunning);
    }

    if (!bHasProject || activeProfile == nullptr) {
        if (!shell.BuildDraftProfileName.empty() || shell.bBuildProfileDraftDirty) {
            SyncBuildDockDraftFromProfile(shell, nullptr);
        }
        ApplyBuildDockDraftToWidgets(shell, nullptr);
    } else if (shell.BuildDraftProfileName != activeProfileName || !shell.bBuildProfileDraftDirty) {
        SyncBuildDockDraftFromProfile(shell, activeProfile);
        ApplyBuildDockDraftToWidgets(shell, activeProfile);
    } else {
        const bool bShowWindowsSettings = activeProfile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop;
        const bool bShowAndroidSettings = activeProfile->TargetPlatform == EEditorTargetPlatform::Android;
        if (shell.BuildWindowsSettingsGroup) {
            shell.BuildWindowsSettingsGroup->SetVisible(bShowWindowsSettings);
        }
        if (shell.BuildAndroidSettingsGroup) {
            shell.BuildAndroidSettingsGroup->SetVisible(bShowAndroidSettings);
        }
    }

    if (shell.BuildWindowsGeneratorComboBox) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop &&
            !bBuildRunning;
        shell.BuildWindowsGeneratorComboBox->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidAbiComboBox) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidAbiComboBox->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidApiComboBox) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidApiComboBox->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidStlComboBox) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidStlComboBox->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidSdkRootEditor) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidSdkRootEditor->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidNdkRootEditor) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidNdkRootEditor->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidSdkBrowseButton) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidSdkBrowseButton->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidSdkClearButton) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidSdkClearButton->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidNdkBrowseButton) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidNdkBrowseButton->SetDisabled(!bEnable);
    }
    if (shell.BuildAndroidNdkClearButton) {
        const bool bEnable =
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::Android &&
            !bBuildRunning;
        shell.BuildAndroidNdkClearButton->SetDisabled(!bEnable);
    }
    if (shell.BuildApplyProfileButton) {
        shell.BuildApplyProfileButton->SetDisabled(
            !bHasProject || activeProfile == nullptr || bBuildRunning || !shell.bBuildProfileDraftDirty);
    }
    if (shell.BuildReprobeButton) {
        shell.BuildReprobeButton->SetDisabled(!bHasProject || activeProfile == nullptr || bBuildRunning);
    }
    if (shell.BuildConfigureButton) {
        shell.BuildConfigureButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
    }
    if (shell.BuildRunButton) {
        shell.BuildRunButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
    }
    if (shell.BuildCleanButton) {
        shell.BuildCleanButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
    }
    if (shell.BuildRebuildButton) {
        shell.BuildRebuildButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
    }
    if (shell.BuildSettingsButton) {
        shell.BuildSettingsButton->SetDisabled(!bHasProject);
    }
    if (shell.BuildRevealButton) {
        shell.BuildRevealButton->SetDisabled(!bHasProject || activeProfileName.empty());
    }
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
        RegisterEditorDefaultStringTables();
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
            UpdateBuildDockActions(Shell_, *appPtr, lockedWorkspace);
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
        bWorkspaceRestoreCompleted_ = false;

        app.SetApplicationTitle(EditorText("App.Title", "ImWidgetV4 Editor").Resolve());
        app.SetApplicationIcon(app.GetCoreIconBrush(ECoreIcon::Settings));
        if (Shell_.UndoButton) {
            Shell_.UndoButton->SetToolTipText(EditorText("TitleBar.Undo", "Undo"));
            Shell_.UndoButton->OnClicked.AddLambda([weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                if (auto lockedWorkspace = weakWorkspace.lock()) {
                    lockedWorkspace->Undo();
                }
            });
        }
        if (Shell_.RedoButton) {
            Shell_.RedoButton->SetToolTipText(EditorText("TitleBar.Redo", "Redo"));
            Shell_.RedoButton->OnClicked.AddLambda([weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                if (auto lockedWorkspace = weakWorkspace.lock()) {
                    lockedWorkspace->Redo();
                }
            });
        }
        if (Shell_.BuildProfileComboBox) {
            Shell_.BuildProfileComboBox->OnSelectionChanged.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImComboBox& comboBox, int) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        if (comboBox.HasSelection()) {
                            lockedWorkspace->SetActiveBuildProfile(comboBox.GetSelectedText());
                        }
                    }
                });
        }
        if (Shell_.BuildWindowsGeneratorComboBox) {
            Shell_.BuildWindowsGeneratorComboBox->OnSelectionChanged.AddLambda([this](ImComboBox& comboBox, int) {
                if (!Shell_.bBuildProfileDraftSyncing && comboBox.HasSelection()) {
                    Shell_.BuildDraftWindowsGenerator =
                        comboBox.GetSelectedText() == "Default" ? std::string() : comboBox.GetSelectedText();
                    Shell_.bBuildProfileDraftDirty = true;
                }
            });
        }
        if (Shell_.BuildAndroidAbiComboBox) {
            Shell_.BuildAndroidAbiComboBox->OnSelectionChanged.AddLambda([this](ImComboBox& comboBox, int) {
                if (!Shell_.bBuildProfileDraftSyncing && comboBox.HasSelection()) {
                    Shell_.BuildDraftAndroidAbi = comboBox.GetSelectedText();
                    Shell_.bBuildProfileDraftDirty = true;
                }
            });
        }
        if (Shell_.BuildAndroidApiComboBox) {
            Shell_.BuildAndroidApiComboBox->OnSelectionChanged.AddLambda([this](ImComboBox& comboBox, int) {
                if (!Shell_.bBuildProfileDraftSyncing && comboBox.HasSelection()) {
                    Shell_.BuildDraftAndroidApi = comboBox.GetSelectedText();
                    Shell_.bBuildProfileDraftDirty = true;
                }
            });
        }
        if (Shell_.BuildAndroidStlComboBox) {
            Shell_.BuildAndroidStlComboBox->OnSelectionChanged.AddLambda([this](ImComboBox& comboBox, int) {
                if (!Shell_.bBuildProfileDraftSyncing && comboBox.HasSelection()) {
                    Shell_.BuildDraftAndroidStl = comboBox.GetSelectedText();
                    Shell_.bBuildProfileDraftDirty = true;
                }
            });
        }
        if (Shell_.BuildAndroidSdkRootEditor) {
            Shell_.BuildAndroidSdkRootEditor->OnTextCommitted.AddLambda([this](ImEditableText&, const std::string& text) {
                if (!Shell_.bBuildProfileDraftSyncing) {
                    Shell_.BuildDraftAndroidSdkRoot = text.empty()
                        ? std::string()
                        : std::filesystem::path(text).lexically_normal().string();
                    Shell_.bBuildProfileDraftDirty = true;
                }
            });
        }
        if (Shell_.BuildAndroidNdkRootEditor) {
            Shell_.BuildAndroidNdkRootEditor->OnTextCommitted.AddLambda([this](ImEditableText&, const std::string& text) {
                if (!Shell_.bBuildProfileDraftSyncing) {
                    Shell_.BuildDraftAndroidNdkRoot = text.empty()
                        ? std::string()
                        : std::filesystem::path(text).lexically_normal().string();
                    Shell_.bBuildProfileDraftDirty = true;
                }
            });
        }
        if (Shell_.BuildApplyProfileButton) {
            Shell_.BuildApplyProfileButton->OnClicked.AddLambda(
                [this](ImButton&) {
                    if (!WorkspaceController_) {
                        return;
                    }

                    const std::shared_ptr<EditorProject> project = WorkspaceController_->GetProject();
                    if (!project) {
                        return;
                    }

                    const std::string activeProfileName = WorkspaceController_->GetActiveBuildProfileName();
                    const FEditorBuildProfile* activeProfile = project->FindBuildProfile(activeProfileName);
                    if (activeProfile == nullptr) {
                        return;
                    }

                    FEditorBuildProfile updatedProfile = *activeProfile;
                    if (updatedProfile.TargetPlatform == EEditorTargetPlatform::WindowsDesktop) {
                        updatedProfile.Generator = Shell_.BuildDraftWindowsGenerator;
                    } else if (updatedProfile.TargetPlatform == EEditorTargetPlatform::Android) {
                        if (!Shell_.BuildDraftAndroidAbi.empty()) {
                            updatedProfile.AndroidSettings.Abi = Shell_.BuildDraftAndroidAbi;
                        }
                        if (!Shell_.BuildDraftAndroidApi.empty()) {
                            try {
                                updatedProfile.AndroidSettings.ApiLevel = std::stoi(Shell_.BuildDraftAndroidApi);
                            } catch (...) {
                                return;
                            }
                        }
                        if (!Shell_.BuildDraftAndroidStl.empty()) {
                            updatedProfile.AndroidSettings.Stl = Shell_.BuildDraftAndroidStl;
                        }
                        updatedProfile.AndroidSettings.SdkRootOverride =
                            Shell_.BuildDraftAndroidSdkRoot.empty()
                                ? std::filesystem::path()
                                : std::filesystem::path(Shell_.BuildDraftAndroidSdkRoot).lexically_normal();
                        updatedProfile.AndroidSettings.NdkRootOverride =
                            Shell_.BuildDraftAndroidNdkRoot.empty()
                                ? std::filesystem::path()
                                : std::filesystem::path(Shell_.BuildDraftAndroidNdkRoot).lexically_normal();
                    }

                    if (WorkspaceController_->UpdateBuildProfile(updatedProfile, true)) {
                        Shell_.bBuildProfileDraftDirty = false;
                    }
                });
        }
        if (Shell_.BuildReprobeButton) {
            Shell_.BuildReprobeButton->OnClicked.AddLambda([this](ImButton&) {
                if (WorkspaceController_) {
                    WorkspaceController_->RequestBuildProfileProbeRefresh();
                }
            });
        }
        if (Shell_.BuildAndroidSdkBrowseButton) {
            Shell_.BuildAndroidSdkBrowseButton->OnClicked.AddLambda([this, appPtr = &app](ImButton&) {
                if (appPtr == nullptr || !WorkspaceController_) {
                    return;
                }

                FOpenFolderDialogOptions options;
                options.Title = "Select Android SDK Root";
                if (!Shell_.BuildDraftAndroidSdkRoot.empty()) {
                    options.InitialDirectory = std::filesystem::path(Shell_.BuildDraftAndroidSdkRoot);
                } else if (const std::shared_ptr<EditorProject> project = WorkspaceController_->GetProject()) {
                    if (const FEditorBuildProfile* activeProfile =
                            project->FindBuildProfile(WorkspaceController_->GetActiveBuildProfileName())) {
                        options.InitialDirectory = ResolveAndroidSdkRootForProfile(*activeProfile);
                    }
                }

                const FPathDialogResult dialogResult = appPtr->OpenFolderDialog(options);
                if (dialogResult.IsAccepted()) {
                    Shell_.BuildDraftAndroidSdkRoot = dialogResult.Path.lexically_normal().string();
                    Shell_.bBuildProfileDraftDirty = true;
                    ApplyBuildDockDraftToWidgets(
                        Shell_,
                        WorkspaceController_->GetProject()
                            ? WorkspaceController_->GetProject()->FindBuildProfile(WorkspaceController_->GetActiveBuildProfileName())
                            : nullptr);
                }
            });
        }
        if (Shell_.BuildAndroidSdkClearButton) {
            Shell_.BuildAndroidSdkClearButton->OnClicked.AddLambda([this](ImButton&) {
                Shell_.BuildDraftAndroidSdkRoot.clear();
                Shell_.bBuildProfileDraftDirty = true;
                ApplyBuildDockDraftToWidgets(
                    Shell_,
                    WorkspaceController_ && WorkspaceController_->GetProject()
                        ? WorkspaceController_->GetProject()->FindBuildProfile(WorkspaceController_->GetActiveBuildProfileName())
                        : nullptr);
            });
        }
        if (Shell_.BuildAndroidNdkBrowseButton) {
            Shell_.BuildAndroidNdkBrowseButton->OnClicked.AddLambda([this, appPtr = &app](ImButton&) {
                if (appPtr == nullptr || !WorkspaceController_) {
                    return;
                }

                FOpenFolderDialogOptions options;
                options.Title = "Select Android NDK Root";
                if (!Shell_.BuildDraftAndroidNdkRoot.empty()) {
                    options.InitialDirectory = std::filesystem::path(Shell_.BuildDraftAndroidNdkRoot);
                } else if (const std::shared_ptr<EditorProject> project = WorkspaceController_->GetProject()) {
                    if (const FEditorBuildProfile* activeProfile =
                            project->FindBuildProfile(WorkspaceController_->GetActiveBuildProfileName())) {
                        options.InitialDirectory = ResolveAndroidNdkRootForProfile(
                            *activeProfile,
                            ResolveAndroidSdkRootForProfile(*activeProfile));
                    }
                }

                const FPathDialogResult dialogResult = appPtr->OpenFolderDialog(options);
                if (dialogResult.IsAccepted()) {
                    Shell_.BuildDraftAndroidNdkRoot = dialogResult.Path.lexically_normal().string();
                    Shell_.bBuildProfileDraftDirty = true;
                    ApplyBuildDockDraftToWidgets(
                        Shell_,
                        WorkspaceController_->GetProject()
                            ? WorkspaceController_->GetProject()->FindBuildProfile(WorkspaceController_->GetActiveBuildProfileName())
                            : nullptr);
                }
            });
        }
        if (Shell_.BuildAndroidNdkClearButton) {
            Shell_.BuildAndroidNdkClearButton->OnClicked.AddLambda([this](ImButton&) {
                Shell_.BuildDraftAndroidNdkRoot.clear();
                Shell_.bBuildProfileDraftDirty = true;
                ApplyBuildDockDraftToWidgets(
                    Shell_,
                    WorkspaceController_ && WorkspaceController_->GetProject()
                        ? WorkspaceController_->GetProject()->FindBuildProfile(WorkspaceController_->GetActiveBuildProfileName())
                        : nullptr);
            });
        }
        if (Shell_.BuildConfigureButton) {
            Shell_.BuildConfigureButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->ConfigureProject();
                    }
                });
        }
        if (Shell_.BuildRunButton) {
            Shell_.BuildRunButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->BuildProject();
                    }
                });
        }
        if (Shell_.BuildCleanButton) {
            Shell_.BuildCleanButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->CleanProject();
                    }
                });
        }
        if (Shell_.BuildRebuildButton) {
            Shell_.BuildRebuildButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->RebuildProject();
                    }
                });
        }
        if (Shell_.BuildSettingsButton) {
            Shell_.BuildSettingsButton->OnClicked.AddLambda(
                [this, appPtr = &app](ImButton&) {
                    if (WorkspaceController_ && appPtr != nullptr) {
                        WorkspaceController_->OpenProjectSettings(*appPtr);
                    }
                });
        }
        if (Shell_.BuildRevealButton) {
            Shell_.BuildRevealButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->RevealProjectBuildDirectory();
                    }
                });
        }
        RebuildEditorTitleBar(app, Shell_, WorkspaceController_);
        UpdateEditorTitleBarActions(app, Shell_, WorkspaceController_);
        UpdateBuildOverviewPanel(Shell_, WorkspaceController_);
        UpdateBuildDockActions(Shell_, app, WorkspaceController_);
        app.SetRootWidget(Shell_.Root);
    }

    bool InitializeApplication(ImApplication&, ImApplicationBackend&) override
    {
        return true;
    }

    void Tick(ImApplication&, const FFrameInfo&) override
    {
        RestoreWorkspaceAfterFirstFrame();
        if (WorkspaceController_) {
            WorkspaceController_->TickBackgroundTasks();
        }
        if (WorkspaceController_) {
            // Keep undo/redo availability responsive without rebuilding the full shell tree.
            // The title text is updated on project state changes via RebuildEditorTitleBar.
            if (BoundApplication_ != nullptr) {
                UpdateEditorTitleBarActions(*BoundApplication_, Shell_, WorkspaceController_);
            }
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
    void RestoreWorkspaceAfterFirstFrame()
    {
        if (bWorkspaceRestoreCompleted_ || WorkspaceController_ == nullptr) {
            return;
        }

        bWorkspaceRestoreCompleted_ = true;
        const std::filesystem::path defaultWorkspaceDirectory = GetDefaultEditorWorkspaceDirectory();
        if (!WorkspaceController_->LoadWorkspaceState(WorkspaceStatePath_)) {
            WorkspaceController_->SetProjectRoot(defaultWorkspaceDirectory);
        }
        WorkspaceController_->EnsureAtLeastOneSession();
    }

    FEditorShellWidgets Shell_;
    std::shared_ptr<EditorWorkspaceController> WorkspaceController_;
    std::filesystem::path WorkspaceStatePath_;
    ImApplication* BoundApplication_ = nullptr;
    bool bWorkspaceRestoreCompleted_ = false;
};

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FEditorApplicationHostDelegate>();
}

} // namespace ImWidgetV4
