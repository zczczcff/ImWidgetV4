#include "editor/EditorSession.h"
#include "editor/EditorShellHost.h"
#include "editor/EditorLocalization.h"
#include "editor/EditorPaths.h"
#include "editor/EditorTheme.h"
#include "editor/EditorWidgetTreeHost.h"
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
#include <fstream>
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

std::string LoadSavedEditorCulture(const std::filesystem::path& workspaceStatePath)
{
    if (workspaceStatePath.empty() || !std::filesystem::exists(workspaceStatePath)) {
        return {};
    }

    try {
        std::ifstream stream(workspaceStatePath);
        if (!stream) {
            return {};
        }

        nlohmann::ordered_json workspaceState;
        stream >> workspaceState;
        return workspaceState.value("Culture", std::string());
    } catch (...) {
        return {};
    }
}

void SaveEditorCulture(const std::filesystem::path& workspaceStatePath, const std::string& culture)
{
    if (workspaceStatePath.empty() || culture.empty()) {
        return;
    }

    try {
        nlohmann::ordered_json workspaceState = nlohmann::ordered_json::object();
        if (std::filesystem::exists(workspaceStatePath)) {
            std::ifstream input(workspaceStatePath);
            if (input) {
                input >> workspaceState;
            }
        }

        workspaceState["Culture"] = culture;
        if (const std::filesystem::path parentPath = workspaceStatePath.parent_path(); !parentPath.empty()) {
            std::error_code errorCode;
            std::filesystem::create_directories(parentPath, errorCode);
        }

        std::ofstream output(workspaceStatePath);
        if (output) {
            output << workspaceState.dump(2);
        }
    } catch (...) {
    }
}

struct FEditorShellWidgets {
    std::shared_ptr<ImWidget> Root;
    std::shared_ptr<ImTitleBar> TitleBar;
    std::shared_ptr<EditorShellHost> ShellHost;
    std::shared_ptr<ImVerticalSplitter> VerticalShell;
    std::shared_ptr<ImHorizontalSplitter> TopWorkspace;
    std::shared_ptr<ImTabView> LeftDockTabs;
    std::shared_ptr<ImScrollBox> ControlPaletteHost;
    std::shared_ptr<ImTabView> DocumentTabs;
    std::shared_ptr<ImTextOutlineView> ProjectView;
    std::shared_ptr<EditorWidgetTreeHost> WidgetTreeHost;
    std::shared_ptr<ImTextOutlineView> WidgetTreeView;
    std::shared_ptr<ImComboBox> BuildProfileComboBox;
    std::shared_ptr<ImComboBox> BuildWindowsGeneratorComboBox;
    std::shared_ptr<ImComboBox> BuildAndroidAbiComboBox;
    std::shared_ptr<ImComboBox> BuildAndroidApiComboBox;
    std::shared_ptr<ImComboBox> BuildAndroidStlComboBox;
    std::shared_ptr<ImEditableText> BuildAndroidSdkRootEditor;
    std::shared_ptr<ImEditableText> BuildAndroidNdkRootEditor;
    std::shared_ptr<ImButton> BuildConfigureButton;
    std::shared_ptr<ImButton> BuildRegenerateCodeButton;
    std::shared_ptr<ImButton> BuildBuildButton;
    std::shared_ptr<ImButton> BuildRunButton;
    std::shared_ptr<ImButton> BuildCleanButton;
    std::shared_ptr<ImButton> BuildClearCacheButton;
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
    std::shared_ptr<ImButton> TitleBarConfigureButton;
    std::shared_ptr<ImButton> TitleBarRegenerateCodeButton;
    std::shared_ptr<ImButton> TitleBarBuildButton;
    std::shared_ptr<ImButton> TitleBarRunButton;
    std::shared_ptr<ImButton> TitleBarOpenFolderButton;
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

std::shared_ptr<ImImage> MakeTitleBarIcon(const FImageBrush& brush, float size = 16.0f)
{
    auto image = std::make_shared<ImImage>();
    image->SetBrush(brush);
    image->SetDesiredSize(FVector2(size, size));
    image->SetStyle(MakeEditorPlainIconStyle(image->GetStyle()));
    return image;
}

std::shared_ptr<FCompactTitleBarButton> MakeTitleBarTextButton(const FText& text)
{
    auto button = std::make_shared<FCompactTitleBarButton>();
    button->SetStyle(MakeEditorTitleBarButtonStyle());
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
    button->SetStyle(MakeEditorTitleBarIconButtonStyle());
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

FApplicationMenuItem MakeEditorMenuItem(
    const std::string& key,
    const std::string& defaultText,
    bool bEnabled,
    std::function<void()> onInvoked)
{
    FApplicationMenuItem item;
    item.Text = defaultText;
    item.TextValue = EditorText(key, defaultText);
    item.bEnabled = bEnabled;
    item.OnInvoked = std::move(onInvoked);
    return item;
}

FApplicationMenuItem MakeEditorMenuItem(
    const FText& text,
    bool bEnabled,
    std::function<void()> onInvoked)
{
    FApplicationMenuItem item;
    item.Text = text.GetInvariantText();
    item.TextValue = text;
    item.bEnabled = bEnabled;
    item.OnInvoked = std::move(onInvoked);
    return item;
}

void BindPopupMenuButton(
    ImApplication& application,
    const std::shared_ptr<ImButton>& button,
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
    title->SetTextColor(GetEditorPanelTitleColor());
    return title;
}

std::shared_ptr<ImTextBlock> MakePanelBody(const std::string& text, float fontSize = 14.0f)
{
    auto body = std::make_shared<ImTextBlock>();
    body->SetText(text);
    body->SetWrapText(false);
    body->SetFontSize(fontSize);
    body->SetTextColor(GetEditorPanelBodyColor());
    return body;
}

std::shared_ptr<ImTextList> MakePanelTextList(const std::vector<FText>& items)
{
    auto list = std::make_shared<ImTextList>();
    list->SetStyle(MakeEditorCodeTextListStyle(list->GetStyle(), FMargin(14.0f), FVector2(0.0f, 120.0f)));
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

std::shared_ptr<ImWidget> BuildControlPalettePanel()
{
    auto host = std::make_shared<ImScrollBox>();
    host->SetStyle(MakeEditorHostScrollStyle(host->GetStyle(), FMargin(6.0f)));
    host->SetContent(BuildWidgetPaletteView());
    return host;
}

std::shared_ptr<ImTextOutlineView> BuildProjectViewPanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeEditorDockOutlineStyle(outline->GetStyle()));
    return outline;
}

std::shared_ptr<ImTextOutlineView> BuildWidgetTreePanel()
{
    auto outline = std::make_shared<ImTextOutlineView>();
    outline->SetSupportsKeyboardFocus(true);
    outline->SetStyle(MakeEditorDockOutlineStyle(outline->GetStyle()));
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

    auto regenerateCodeButton = std::make_shared<ImButton>();
    regenerateCodeButton->SetText(EditorText("Build.RegenerateCode", "Regenerate Code"));

    auto buildButton = std::make_shared<ImButton>();
    buildButton->SetText(EditorText("Build.Build", "Build"));

    auto runButton = std::make_shared<ImButton>();
    runButton->SetText(EditorText("Build.Run", "Run"));

    auto cleanButton = std::make_shared<ImButton>();
    cleanButton->SetText(EditorText("Build.Clean", "Clean"));

    auto clearCacheButton = std::make_shared<ImButton>();
    clearCacheButton->SetText(EditorText("Build.ClearCache", "Clear Cache"));

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
    buttonRow->AddChildFill(regenerateCodeButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(buildButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(runButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(cleanButton, 1.0f, FMargin(0.0f));
    buttonRow->AddChildFill(clearCacheButton, 1.0f, FMargin(0.0f));
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
    shell.BuildRegenerateCodeButton = regenerateCodeButton;
    shell.BuildBuildButton = buildButton;
    shell.BuildRunButton = runButton;
    shell.BuildCleanButton = cleanButton;
    shell.BuildClearCacheButton = clearCacheButton;
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
    title->SetTextColor(GetEditorPanelTitleColor());
    if (ImCanvasPanelSlot* slot = canvas->AddChildAt(title, FVector2(0.08f, 0.08f))) {
        slot->SetAutoSize(true);
    }

    auto hint = std::make_shared<ImTextBlock>();
    hint->SetName("HintText");
    hint->SetText(EditorText("App.InitialHint", "Drag widgets from the left palette into the designer surface."));
    hint->SetFontSize(18.0f);
    hint->SetWrapText(false);
    hint->SetTextColor(GetEditorPanelBodyColor());
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

    tabView->SetStyle(MakeEditorDockTabStyle(tabView->GetStyle()));

    tabView->AddTab(EditorText("Dock.Controls", "Controls"), BuildControlPalettePanel());
    auto projectView = BuildProjectViewPanel();
    tabView->AddTab(EditorText("Dock.Project", "Project"), projectView);
    tabView->AddTab(EditorText("Dock.Build", "Build"), BuildBuildDockPanel(shell));
    auto widgetTreeView = BuildWidgetTreePanel();
    auto widgetTreeHost = std::make_shared<EditorWidgetTreeHost>();
    widgetTreeHost->SetWidgetTreeView(widgetTreeView);
    shell.WidgetTreeHost = widgetTreeHost;
    shell.WidgetTreeView = widgetTreeView;
    tabView->AddTab(EditorText("Dock.WidgetTree", "Widget Tree"), widgetTreeHost);
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
    titleBar->SetStyle(MakeEditorTitleBarStyle(titleBar->GetStyle()));

    auto titleIcon = MakeTitleBarIcon(FImageBrush(), 18.0f);
    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetText(EditorText("App.Title", "ImWidgetV4 Editor"));
    titleText->SetFontSize(16.0f);
    titleText->SetWrapText(false);
    titleText->SetTextColor(GetEditorPanelTitleColor());

    auto titleBarProfileStatusText = std::make_shared<ImTextBlock>();
    titleBarProfileStatusText->SetText("");
    titleBarProfileStatusText->SetFontSize(12.0f);
    titleBarProfileStatusText->SetWrapText(false);
    titleBarProfileStatusText->SetTextColor(GetEditorTitleBarMutedTextColor());

    titleBar->AddLeadingItem(titleIcon);
    titleBar->AddLeadingItem(titleText);
    titleBar->AddLeadingItem(titleBarProfileStatusText);

    auto undoButton = MakeTitleBarIconButton(FImageBrush(), EditorText("TitleBar.Undo", "Undo"));
    auto redoButton = MakeTitleBarIconButton(FImageBrush(), EditorText("TitleBar.Redo", "Redo"));
    auto titleBarConfigureButton = MakeTitleBarIconButton(FImageBrush(), EditorText("Build.Configure", "Configure"));
    auto titleBarRegenerateCodeButton = MakeTitleBarIconButton(FImageBrush(), EditorText("Build.RegenerateCode", "Regenerate Code"));
    auto titleBarBuildButton = MakeTitleBarIconButton(FImageBrush(), EditorText("Build.Build", "Build"));
    auto titleBarRunButton = MakeTitleBarIconButton(FImageBrush(), EditorText("Build.Run", "Run"));
    auto titleBarOpenFolderButton = MakeTitleBarIconButton(FImageBrush(), EditorText("TitleBar.OpenFolder", "Open Folder"));

    auto verticalShell = std::make_shared<ImVerticalSplitter>();
    verticalShell->SetSupportsKeyboardFocus(false);
    verticalShell->SetPartMinSize(0, 300.0f);

    auto topWorkspace = std::make_shared<ImHorizontalSplitter>();
    topWorkspace->SetSupportsKeyboardFocus(false);

    topWorkspace->SetSplitterStyle(MakeEditorHorizontalSplitterStyle(topWorkspace->GetSplitterStyle()));

    verticalShell->SetSplitterStyle(MakeEditorVerticalSplitterStyle(verticalShell->GetSplitterStyle()));

    auto leftDock = BuildLeftDockTabs(shell);
    auto projectView = std::dynamic_pointer_cast<ImTextOutlineView>(leftDock->GetTab(1)->Content);
    auto buildOverviewText = std::dynamic_pointer_cast<ImTextList>(leftDock->GetTab(2)->Content);

    auto documentTabs = std::make_shared<ImTabView>();
    documentTabs->SetSupportsKeyboardFocus(true);
    FTabViewStyle tabStyle = MakeEditorWorkspaceTabStyle(documentTabs->GetStyle());
    tabStyle.TabHeight = 36.0f;
    tabStyle.TabMinWidth = 150.0f;
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
    shell.VerticalShell = verticalShell;
    shell.TopWorkspace = topWorkspace;
    shell.LeftDockTabs = leftDock;
    shell.ControlPaletteHost = std::dynamic_pointer_cast<ImScrollBox>(leftDock->GetTab(0)->Content);
    shell.DocumentTabs = documentTabs;
    shell.ProjectView = projectView;
    shell.BuildOverviewText = buildOverviewText;
    shell.DetailsView = detailsView;
    shell.OutputText = outputText;
    shell.TitleBarIcon = titleIcon;
    shell.TitleBarText = titleText;
    shell.TitleBarProfileStatusText = titleBarProfileStatusText;
    shell.TitleBarConfigureButton = titleBarConfigureButton;
    shell.TitleBarRegenerateCodeButton = titleBarRegenerateCodeButton;
    shell.TitleBarBuildButton = titleBarBuildButton;
    shell.TitleBarRunButton = titleBarRunButton;
    shell.TitleBarOpenFolderButton = titleBarOpenFolderButton;
    shell.UndoButton = undoButton;
    shell.RedoButton = redoButton;
    return shell;
}

std::vector<FApplicationMenuItem> BuildFileMenuItems(
    ImApplication& app,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    return {
        MakeEditorMenuItem("Menu.New", "New", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->NewDocument();
            }
        }),
        MakeEditorMenuItem("Menu.OpenEllipsis", "Open...", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenDocument(app);
            }
        }),
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.Save", "Save", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SaveDocument(app);
            }
        }),
        MakeEditorMenuItem("Menu.SaveAs", "Save As...", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SaveDocumentAs(app);
            }
        }),
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Common.Close", "Close", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->CloseActiveDocument(app);
            }
        })
    };
}

std::vector<FApplicationMenuItem> BuildSimpleMenuItems(const std::string& menuName)
{
    std::vector<FApplicationMenuItem> items;
    items.push_back(MakeEditorMenuItem(FText::FromString(menuName + " Action"), true, []() {}));
    items.push_back(FApplicationMenuItem {"", FImageBrush(), {}, true, true, {}});
    items.push_back(MakeEditorMenuItem("Menu.ComingSoon", "Coming Soon", false, {}));
    return items;
}

std::vector<FApplicationMenuItem> BuildViewMenuItems(
    ImApplication& app,
    const std::function<void()>& onCultureChanged,
    const std::function<void()>& onThemeChanged)
{
    auto makeCultureItem =
        [&app, onCultureChanged](const std::string& culture, const FText& label) {
            return MakeEditorMenuItem(
                label,
                true,
                [&app, culture, onCultureChanged]() {
                    if (app.SetCulture(culture) && onCultureChanged) {
                        onCultureChanged();
                    }
                });
        };

    auto makeThemeItem =
        [&app, onThemeChanged](const std::string& themeName) {
            return MakeEditorMenuItem(
                FText::FromString(themeName),
                true,
                [&app, themeName, onThemeChanged]() {
                    if (app.SetActiveTheme(themeName) && onThemeChanged) {
                        onThemeChanged();
                    }
                });
        };

    std::vector<FApplicationMenuItem> languageItems = {
        makeCultureItem("en-US", EditorText("Language.English", "English")),
        makeCultureItem("zh-CN", EditorText("Language.ChineseSimplified", "Chinese (Simplified)"))
    };

    std::vector<FApplicationMenuItem> themeItems;
    for (const FThemePack& themePack : app.GetThemePacks()) {
        themeItems.push_back(makeThemeItem(themePack.Name));
    }

    FApplicationMenuItem languageItem;
    languageItem.Text = "Language";
    languageItem.TextValue = EditorText("Menu.Language", "Language");
    languageItem.SubItems = std::move(languageItems);
    languageItem.bEnabled = true;

    FApplicationMenuItem themeItem;
    themeItem.Text = "Theme";
    themeItem.TextValue = EditorText("Menu.Theme", "Theme");
    themeItem.SubItems = std::move(themeItems);
    themeItem.bEnabled = !themeItem.SubItems.empty();

    return {
        languageItem,
        themeItem,
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.ComingSoon", "Coming Soon", false, {})
    };
}

std::vector<FApplicationMenuItem> BuildEditMenuItems(const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    return {
        MakeEditorMenuItem("Menu.Cut", "Cut", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->CutSelectedWidget();
            }
        }),
        MakeEditorMenuItem("Menu.Copy", "Copy", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->CopySelectedWidget();
            }
        }),
        MakeEditorMenuItem("Menu.Paste", "Paste", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->PasteCopiedWidget();
            }
        }),
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("TitleBar.Undo", "Undo", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->Undo();
            }
        }),
        MakeEditorMenuItem("TitleBar.Redo", "Redo", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->Redo();
            }
        }),
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.Duplicate", "Duplicate", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->DuplicateSelectedWidget();
            }
        }),
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.ComingSoon", "Coming Soon", false, {})
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
                ? EditorText("Build.Status", "Status").Resolve() + ": " +
                    (buildStatus.empty() ? EditorText("Build.Running", "Running...").Resolve() : buildStatus)
                : EditorText("Build.Status", "Status").Resolve() + ": " + EditorText("Build.Idle", "Idle").Resolve(),
            {},
            {},
            false,
            false,
            {}
        },
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Build.ConfigureActiveProfile", "Configure Active Profile", bHasProject && !bBuildRunning, [workspaceController]() {
            if (workspaceController) {
                workspaceController->ConfigureProject();
            }
        }),
        MakeEditorMenuItem("Build.RegenerateProjectCode", "Regenerate Project Code", bHasProject && !bBuildRunning, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RegenerateProjectCode();
            }
        }),
        MakeEditorMenuItem("Build.BuildActiveProfile", "Build Active Profile", bHasProject && !bBuildRunning, [workspaceController]() {
            if (workspaceController) {
                workspaceController->BuildProject();
            }
        }),
        MakeEditorMenuItem("Build.CleanActiveProfile", "Clean Active Profile", bHasProject && !bBuildRunning, [workspaceController]() {
            if (workspaceController) {
                workspaceController->CleanProject();
            }
        }),
        MakeEditorMenuItem("Build.ClearCacheActiveProfile", "Clear Active Profile Cache", bHasProject && !bBuildRunning, [workspaceController]() {
            if (workspaceController) {
                workspaceController->ClearBuildCache();
            }
        }),
        MakeEditorMenuItem("Build.RebuildActiveProfile", "Rebuild Active Profile", bHasProject && !bBuildRunning, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RebuildProject();
            }
        }),
        FApplicationMenuItem {"", {}, {}, true, true, {}}
    };

    const std::string activeProfileName =
        workspaceController ? workspaceController->GetActiveBuildProfileName() : std::string();
    if (!activeProfileName.empty()) {
        items.push_back(FApplicationMenuItem {
            EditorText("Build.ActiveProfile", "Active Profile").Resolve() + ": " + activeProfileName,
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
                    ? (probeReport.bReady
                        ? EditorText("Build.Ready", "Ready").Resolve()
                        : EditorText("Build.NeedsSetup", "Needs Setup").Resolve())
                    : (bProbeRefreshing
                        ? EditorText("Build.Refreshing", "Refreshing").Resolve()
                        : EditorText("Build.Unknown", "Unknown").Resolve());
                std::vector<FApplicationMenuItem> profileSubItems = {
                    MakeEditorMenuItem(
                        profile.Name == activeProfileName
                            ? EditorText("Menu.ActiveProfile", "Active Profile")
                            : EditorText("Menu.SetActiveProfile", "Set Active Profile"),
                        !bBuildRunning,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->SetActiveBuildProfile(profileName);
                            }
                        }),
                    MakeEditorMenuItem(
                        "Menu.ConfigureThisProfile",
                        "Configure This Profile",
                        !bBuildRunning,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->ConfigureProject(profileName);
                            }
                        }),
                    MakeEditorMenuItem(
                        "Menu.BuildThisProfile",
                        "Build This Profile",
                        !bBuildRunning,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->BuildProject(profileName);
                            }
                        }),
                    MakeEditorMenuItem(
                        "Menu.CleanThisProfile",
                        "Clean This Profile",
                        !bBuildRunning,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->CleanProject(profileName);
                            }
                        }),
                    MakeEditorMenuItem(
                        "Menu.ClearThisProfileCache",
                        "Clear This Profile Cache",
                        !bBuildRunning,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->ClearBuildCache(profileName);
                            }
                        }),
                    MakeEditorMenuItem(
                        "Menu.RebuildThisProfile",
                        "Rebuild This Profile",
                        !bBuildRunning,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->RebuildProject(profileName);
                            }
                        }),
                    MakeEditorMenuItem(
                        "Menu.RevealBuildFolder",
                        "Reveal Build Folder",
                        true,
                        [workspaceController, profileName = profile.Name]() {
                            if (workspaceController) {
                                workspaceController->RevealProjectBuildDirectory(profileName);
                            }
                        }),
                    FApplicationMenuItem {"", {}, {}, true, true, {}},
                    FApplicationMenuItem {
                        EditorText("ProjectSettings.ProbeTarget", "Target").Resolve() + ": " + GetTargetPlatformDisplayName(profile.TargetPlatform),
                        {},
                        {},
                        false,
                        false,
                        {}
                    },
                    FApplicationMenuItem {
                        EditorText("Build.Configuration", "Configuration").Resolve() + ": " + profile.Configuration,
                        {},
                        {},
                        false,
                        false,
                        {}
                    },
                    FApplicationMenuItem {
                        EditorText("ProjectSettings.EnvironmentProbe", "Environment Probe").Resolve() + ": " + readinessLabel,
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

    items.push_back(MakeEditorMenuItem("Menu.RevealBuildFolder", "Reveal Build Folder", bHasProject, [workspaceController]() {
        if (workspaceController) {
            workspaceController->RevealProjectBuildDirectory();
        }
    }));

    items.push_back(FApplicationMenuItem {"", {}, {}, true, true, {}});
    items.push_back(FApplicationMenuItem {
        bHasProject && !activeProfileName.empty()
            ? EditorText("Build.Profile", "Profile").Resolve() + ": " + activeProfileName
            : EditorText("Build.NoActiveBuildProfile", "No active build profile").Resolve(),
        {},
        {},
        false,
        false,
        {}
    });
    items.push_back(FApplicationMenuItem {
        bHasProject
            ? EditorText("Build.WorkflowUsesSavedProjectProfileSettings", "Build workflow uses saved project profile settings").Resolve()
            : EditorText("Build.DirectoryNotAvailable", "Build directory not available").Resolve(),
        {},
        {},
        false,
        false,
        {}
    });

    return items;
}

std::vector<FApplicationMenuItem> BuildOpenFolderMenuItems(
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    const bool bHasProject = workspaceController && !workspaceController->GetProjectRoot().empty();
    const bool bCanOpenDebugExecutableDirectory =
        workspaceController && workspaceController->CanRevealExecutableDirectoryForConfiguration("Debug");
    const bool bCanOpenReleaseExecutableDirectory =
        workspaceController && workspaceController->CanRevealExecutableDirectoryForConfiguration("Release");

    return {
        MakeEditorMenuItem(
            "Menu.RevealProjectRoot",
            "Reveal Project Root",
            bHasProject,
            [workspaceController]() {
                if (workspaceController) {
                    workspaceController->RevealProjectItemInExplorer(workspaceController->GetProjectRoot());
                }
            }),
        FApplicationMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem(
            "Menu.OpenDebugExecutableFolder",
            "Open Debug Executable Folder",
            bCanOpenDebugExecutableDirectory,
            [workspaceController]() {
                if (workspaceController) {
                    workspaceController->RevealExecutableDirectoryForConfiguration("Debug");
                }
            }),
        MakeEditorMenuItem(
            "Menu.OpenReleaseExecutableFolder",
            "Open Release Executable Folder",
            bCanOpenReleaseExecutableDirectory,
            [workspaceController]() {
                if (workspaceController) {
                    workspaceController->RevealExecutableDirectoryForConfiguration("Release");
                }
            })
    };
}

std::vector<FPopupMenuItem> BuildProjectMenuItems(
    ImApplication& app,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    return {
        MakeEditorMenuItem("Menu.NewAppProject", "New App Project...", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->NewAppProject(app);
            }
        }),
        MakeEditorMenuItem("Menu.OpenAppProject", "Open App Project...", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenAppProject(app);
            }
        }),
        FPopupMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.OpenProjectRoot", "Open Project Root...", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->SelectProjectRoot(app);
            }
        }),
        MakeEditorMenuItem("Menu.ProjectSettings", "Project Settings...", workspaceController && workspaceController->GetProject(), [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->OpenProjectSettings(app);
            }
        }),
        FPopupMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.GenerateCpp", "Generate C++...", true, [&app, workspaceController]() {
            if (workspaceController) {
                workspaceController->GenerateActiveDocumentCpp(app);
            }
        }),
        FPopupMenuItem {"", {}, {}, true, true, {}},
        MakeEditorMenuItem("Menu.NewUIDocument", "New UI Document...", workspaceController && !workspaceController->GetProjectRoot().empty(), [&app, workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->CreateDocumentInDirectory(app, workspaceController->GetProjectRoot());
            }
        }),
        MakeEditorMenuItem("Menu.NewFolder", "New Folder", workspaceController && !workspaceController->GetProjectRoot().empty(), [&app, workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->CreateFolderInDirectory(app, workspaceController->GetProjectRoot());
            }
        }),
        MakeEditorMenuItem("Menu.RefreshProjectTree", "Refresh Project Tree", true, [workspaceController]() {
            if (workspaceController) {
                workspaceController->RefreshProjectTree();
            }
        }),
        MakeEditorMenuItem("Menu.RevealProjectRoot", "Reveal Project Root", workspaceController && !workspaceController->GetProjectRoot().empty(), [workspaceController]() {
            if (workspaceController && !workspaceController->GetProjectRoot().empty()) {
                workspaceController->RevealProjectItemInExplorer(workspaceController->GetProjectRoot());
            }
        }),
        FPopupMenuItem {"", {}, {}, true, true, {}},
        FPopupMenuItem {
            workspaceController && !workspaceController->GetProjectRoot().empty()
                ? workspaceController->GetProjectRoot().string()
                : EditorText("Project.WorkspaceRootNotConfigured", "Workspace root not configured").Resolve(),
            {},
            {},
            false,
            false,
            {}
        }
    };
}

void RefreshLocalizedEditorShell(
    ImApplication& app,
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController);

void UpdateEditorTitleBarActions(
    ImApplication& app,
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController);

void ApplyEditorThemeToShell(
    ImApplication& app,
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    SetEditorActiveTheme(app.GetActiveThemeName(), app.GetStyleSet());

    if (shell.TitleBar) {
        shell.TitleBar->SetStyle(MakeEditorTitleBarStyle(shell.TitleBar->GetStyle()));
    }
    if (shell.TitleBarText) {
        shell.TitleBarText->SetTextColor(GetEditorTitleBarTextColor());
    }
    if (shell.TitleBarProfileStatusText) {
        shell.TitleBarProfileStatusText->SetTextColor(GetEditorTitleBarMutedTextColor());
    }
    if (shell.UndoButton) {
        shell.UndoButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.RedoButton) {
        shell.RedoButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.TitleBarConfigureButton) {
        shell.TitleBarConfigureButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.TitleBarRegenerateCodeButton) {
        shell.TitleBarRegenerateCodeButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.TitleBarBuildButton) {
        shell.TitleBarBuildButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.TitleBarRunButton) {
        shell.TitleBarRunButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.TitleBarOpenFolderButton) {
        shell.TitleBarOpenFolderButton->SetStyle(MakeEditorTitleBarIconButtonStyle());
    }
    if (shell.VerticalShell) {
        shell.VerticalShell->SetSplitterStyle(MakeEditorVerticalSplitterStyle(shell.VerticalShell->GetSplitterStyle()));
    }
    if (shell.TopWorkspace) {
        shell.TopWorkspace->SetSplitterStyle(MakeEditorHorizontalSplitterStyle(shell.TopWorkspace->GetSplitterStyle()));
    }
    if (shell.LeftDockTabs) {
        shell.LeftDockTabs->SetStyle(MakeEditorDockTabStyle(shell.LeftDockTabs->GetStyle()));
    }
    if (shell.ControlPaletteHost) {
        shell.ControlPaletteHost->SetStyle(MakeEditorHostScrollStyle(shell.ControlPaletteHost->GetStyle(), FMargin(6.0f)));
    }
    if (shell.DocumentTabs) {
        FTabViewStyle tabStyle = MakeEditorWorkspaceTabStyle(shell.DocumentTabs->GetStyle());
        tabStyle.TabHeight = 36.0f;
        tabStyle.TabMinWidth = 150.0f;
        shell.DocumentTabs->SetStyle(tabStyle);
    }
    if (shell.ProjectView) {
        shell.ProjectView->SetStyle(MakeEditorDockOutlineStyle(shell.ProjectView->GetStyle()));
    }
    if (shell.WidgetTreeView) {
        shell.WidgetTreeView->SetStyle(MakeEditorDockOutlineStyle(shell.WidgetTreeView->GetStyle()));
    }
    if (shell.BuildOverviewText) {
        shell.BuildOverviewText->SetStyle(
            MakeEditorCodeTextListStyle(
                shell.BuildOverviewText->GetStyle(),
                FMargin(14.0f),
                FVector2(0.0f, 120.0f)));
    }
    if (shell.OutputText) {
        shell.OutputText->SetStyle(
            MakeEditorCodeTextListStyle(
                shell.OutputText->GetStyle(),
                FMargin(14.0f),
                FVector2(0.0f, 120.0f)));
    }

    using namespace ImWidgetV4Editor::PropertyEditorWidgets;
    if (shell.BuildProfileComboBox) {
        ApplyInspectorComboBoxStyle(*shell.BuildProfileComboBox);
    }
    if (shell.BuildWindowsGeneratorComboBox) {
        ApplyInspectorComboBoxStyle(*shell.BuildWindowsGeneratorComboBox);
    }
    if (shell.BuildAndroidAbiComboBox) {
        ApplyInspectorComboBoxStyle(*shell.BuildAndroidAbiComboBox);
    }
    if (shell.BuildAndroidApiComboBox) {
        ApplyInspectorComboBoxStyle(*shell.BuildAndroidApiComboBox);
    }
    if (shell.BuildAndroidStlComboBox) {
        ApplyInspectorComboBoxStyle(*shell.BuildAndroidStlComboBox);
    }
    if (shell.BuildAndroidSdkRootEditor) {
        ApplyInspectorEditableTextStyle(*shell.BuildAndroidSdkRootEditor, shell.BuildAndroidSdkRootEditor->IsDisabled());
    }
    if (shell.BuildAndroidNdkRootEditor) {
        ApplyInspectorEditableTextStyle(*shell.BuildAndroidNdkRootEditor, shell.BuildAndroidNdkRootEditor->IsDisabled());
    }
    if (shell.DetailsView) {
        shell.DetailsView->RebuildPreservingViewState();
    }

    UpdateEditorTitleBarActions(app, shell, workspaceController);
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
            : EditorText("App.Title", "ImWidgetV4 Editor").Resolve();
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
    BindPopupMenuButton(app, viewButton, [&app, &shell, workspaceController]() {
        return BuildViewMenuItems(
            app,
            [&app, &shell, workspaceController]() {
                RefreshLocalizedEditorShell(app, shell, workspaceController);
            },
            [&app, &shell, workspaceController]() {
                RefreshLocalizedEditorShell(app, shell, workspaceController);
            });
    });
    shell.TitleBar->AddLeadingItem(viewButton);

    if (shell.UndoButton) {
        shell.UndoButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Undo, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.UndoButton);
    }
    if (shell.RedoButton) {
        shell.RedoButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Redo, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.RedoButton);
    }

    auto searchButton = MakeTitleBarIconButton(
        app.GetCoreIconBrush(ECoreIcon::Search, GetEditorTitleBarIconColor()),
        EditorText("TitleBar.Search", "Search"));
    BindPopupMenuButton(app, searchButton, []() {
        return BuildSimpleMenuItems("Search");
    });
    shell.TitleBar->AddLeadingItem(searchButton);

    if (shell.TitleBarConfigureButton) {
        shell.TitleBarConfigureButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Configure, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.TitleBarConfigureButton);
    }
    if (shell.TitleBarRegenerateCodeButton) {
        shell.TitleBarRegenerateCodeButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Generate, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.TitleBarRegenerateCodeButton);
    }
    if (shell.TitleBarBuildButton) {
        shell.TitleBarBuildButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Build, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.TitleBarBuildButton);
    }
    if (shell.TitleBarRunButton) {
        shell.TitleBarRunButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::Play, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.TitleBarRunButton);
    }
    if (shell.TitleBarOpenFolderButton) {
        shell.TitleBarOpenFolderButton->SetContent(MakeTitleBarIcon(app.GetCoreIconBrush(ECoreIcon::OpenBuildDirectory, GetEditorTitleBarIconColor()), 16.0f));
        shell.TitleBar->AddLeadingItem(shell.TitleBarOpenFolderButton);
    }
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
            app.GetCoreIconBrush(ECoreIcon::Undo, bCanUndo ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor()),
            16.0f));
    }

    if (shell.RedoButton && shell.bLastCanRedo != bCanRedo) {
        shell.bLastCanRedo = bCanRedo;
        shell.RedoButton->SetDisabled(!bCanRedo);
        shell.RedoButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Redo, bCanRedo ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor()),
            16.0f));
    }

    if (shell.TitleBarProfileStatusText) {
        std::string statusText = EditorText("Build.NoActiveBuildProfile", "No active build profile").Resolve();
        FColor statusColor = GetEditorTitleBarMutedTextColor();

        if (workspaceController) {
            const std::string activeProfileName = workspaceController->GetActiveBuildProfileName();
            if (!activeProfileName.empty()) {
                statusText = activeProfileName;
                if (workspaceController->IsBuildTaskRunning()) {
                    const std::string buildStatus = workspaceController->GetBuildTaskStatusText();
                    statusText += " | " + (buildStatus.empty() ? EditorText("Build.Running", "Running...").Resolve() : buildStatus);
                    statusColor = GetEditorAccentColor();
                } else {
                    FEnvironmentProbeReport probeReport;
                    if (workspaceController->TryGetBuildProfileProbeReport(activeProfileName, probeReport)) {
                        if (probeReport.bReady) {
                            statusText += " | " + EditorText("Build.Ready", "Ready").Resolve();
                            statusColor = GetEditorSuccessColor();
                        } else {
                            statusText += " | " + EditorText("Build.NeedsSetup", "Needs Setup").Resolve();
                            statusColor = GetEditorWarningColor();
                        }
                    } else if (workspaceController->IsBuildProfileProbeRefreshing(activeProfileName)) {
                        statusText += " | " + EditorText("Build.Checking", "Checking...").Resolve();
                        statusColor = GetEditorAccentColor();
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
            EditorText("Build.NoProjectLoaded", "No project loaded.").Resolve(),
            "",
            EditorText("Build.OpenOrCreateProjectHint", "Open or create an app project to inspect toolchain readiness.").Resolve()
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
    lines.push_back(EditorText("Build.ActiveProfile", "Active Profile").Resolve() + ": " +
        (activeProfileName.empty() ? EditorText("Common.None", "None").Resolve() : activeProfileName));
    lines.push_back(EditorText("Build.Status", "Status").Resolve() + ": " + (workspaceController->IsBuildTaskRunning()
        ? workspaceController->GetBuildTaskStatusText()
        : EditorText("Build.Idle", "Idle").Resolve()));
    if (bProbeRefreshing) {
        lines.push_back(EditorText("Build.ProbeReady", "Probe Ready").Resolve() + ": " + EditorText("Build.Refreshing", "Refreshing").Resolve() + "...");
    } else if (!probeReport.Items.empty()) {
        lines.push_back(EditorText("Build.ProbeReady", "Probe Ready").Resolve() + ": " +
            (probeReport.bReady ? EditorText("Common.Yes", "Yes").Resolve() : EditorText("Common.No", "No").Resolve()));
    } else {
        lines.push_back(EditorText("Build.ProbeReady", "Probe Ready").Resolve() + ": " + EditorText("Build.Unknown", "Unknown").Resolve());
    }
    lines.push_back("");

    if (activeProfile != nullptr) {
        lines.push_back(EditorText("Build.ProfileDetails", "Profile Details").Resolve() + ":");
        lines.push_back("  " + EditorText("ProjectSettings.ProbeTarget", "Target").Resolve() + ": " + GetTargetPlatformDisplayName(activeProfile->TargetPlatform));
        lines.push_back("  " + EditorText("Build.Configuration", "Configuration").Resolve() + ": " + activeProfile->Configuration);
        lines.push_back("  " + EditorText("Build.Generator", "Generator").Resolve() + ": " +
            (activeProfile->Generator.empty() ? EditorText("Common.Default", "Default").Resolve() : activeProfile->Generator));
        lines.push_back("  " + EditorText("Build.Directory", "Build Directory").Resolve() + ": " + ResolveBuildDirectoryPath(project->GetProjectRoot(), *activeProfile).string());

        if (activeProfile->TargetPlatform == EEditorTargetPlatform::Android) {
            lines.push_back("  Android ABI: " + activeProfile->AndroidSettings.Abi);
            lines.push_back("  Android API: " + std::to_string(activeProfile->AndroidSettings.ApiLevel));
            lines.push_back("  Android STL: " + activeProfile->AndroidSettings.Stl);
            if (!activeProfile->AndroidSettings.SdkRootOverride.empty()) {
                lines.push_back("  " + EditorText("Build.SdkOverride", "SDK Override").Resolve() + ": " + activeProfile->AndroidSettings.SdkRootOverride.string());
            }
            if (!activeProfile->AndroidSettings.NdkRootOverride.empty()) {
                lines.push_back("  " + EditorText("Build.NdkOverride", "NDK Override").Resolve() + ": " + activeProfile->AndroidSettings.NdkRootOverride.string());
            }
        }

        lines.push_back("");
    }

    if (bProbeRefreshing) {
        lines.push_back(EditorText("ProjectSettings.EnvironmentProbe", "Environment Probe").Resolve() + ":");
        lines.push_back("  " + EditorText("Build.RefreshingToolchainStatus", "Refreshing toolchain status...").Resolve());
    } else if (probeReport.Items.empty()) {
        lines.push_back(EditorText("Build.NoProbeDataAvailable", "No probe data available.").Resolve());
    } else {
        std::vector<std::string> missingItems;
        for (const FEnvironmentProbeItem& item : probeReport.Items) {
            if (item.Status == EEnvironmentProbeStatus::Missing) {
                missingItems.push_back(item.Label);
            }
        }

        if (!missingItems.empty()) {
            lines.push_back(EditorText("Build.SetupNeeded", "Setup Needed").Resolve() + ":");
            for (const std::string& missingItem : missingItems) {
                lines.push_back("  " + EditorText("Build.Missing", "Missing").Resolve() + " " + missingItem);
            }
            lines.push_back("  " + EditorText("Build.UseSettingsToUpdateProfile", "Use Settings to update this profile and re-probe.").Resolve());
            lines.push_back("");
        }

        lines.push_back(EditorText("ProjectSettings.EnvironmentProbe", "Environment Probe").Resolve() + ":");
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
        lines.push_back(EditorText("Build.RecentOutput", "Recent Output").Resolve() + ":");
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
    const bool bRunRunning = workspaceController && workspaceController->IsRunTaskRunning();
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
    if (shell.BuildRegenerateCodeButton) {
        shell.BuildRegenerateCodeButton->SetDisabled(!bHasProject || bBuildRunning);
    }
    if (shell.BuildBuildButton) {
        shell.BuildBuildButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
    }
    if (shell.BuildRunButton) {
        const bool bEnableRun =
            bRunRunning ||
            (bHasProject &&
                activeProfile != nullptr &&
                activeProfile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop &&
                !activeProfileName.empty() &&
                !bBuildRunning);
        shell.BuildRunButton->SetDisabled(!bEnableRun);
        shell.BuildRunButton->SetText(bRunRunning
            ? EditorText("Build.Stop", "Stop")
            : EditorText("Build.Run", "Run"));
    }
    if (shell.BuildCleanButton) {
        shell.BuildCleanButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
    }
    if (shell.BuildClearCacheButton) {
        shell.BuildClearCacheButton->SetDisabled(!bHasProject || activeProfileName.empty() || bBuildRunning);
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

    const bool bCanRunActiveProfile =
        bRunRunning ||
        (bHasProject &&
            activeProfile != nullptr &&
            activeProfile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop &&
            !activeProfileName.empty() &&
            !bBuildRunning);
    const bool bCanUseActiveBuildProfile = bHasProject && !activeProfileName.empty() && !bBuildRunning;
    if (shell.TitleBarConfigureButton) {
        shell.TitleBarConfigureButton->SetDisabled(!bCanUseActiveBuildProfile);
        shell.TitleBarConfigureButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Configure, bCanUseActiveBuildProfile ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor()),
            16.0f));
    }
    if (shell.TitleBarRegenerateCodeButton) {
        const bool bCanRegenerateCode = bHasProject && !bBuildRunning;
        shell.TitleBarRegenerateCodeButton->SetDisabled(!bCanRegenerateCode);
        shell.TitleBarRegenerateCodeButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Generate, bCanRegenerateCode ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor()),
            16.0f));
    }
    if (shell.TitleBarBuildButton) {
        shell.TitleBarBuildButton->SetDisabled(!bCanUseActiveBuildProfile);
        shell.TitleBarBuildButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(ECoreIcon::Build, bCanUseActiveBuildProfile ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor()),
            16.0f));
    }
    if (shell.TitleBarRunButton) {
        shell.TitleBarRunButton->SetDisabled(!bCanRunActiveProfile);
        shell.TitleBarRunButton->SetToolTipText(bRunRunning
            ? EditorText("Build.Stop", "Stop")
            : EditorText("Build.Run", "Run"));
        shell.TitleBarRunButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(
                bRunRunning ? ECoreIcon::Stop : ECoreIcon::Play,
                bRunRunning
                    ? GetEditorDangerColor()
                    : (bCanRunActiveProfile ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor())),
            16.0f));
    }
    if (shell.TitleBarOpenFolderButton) {
        const bool bCanOpenFolderMenu = bHasProject;
        shell.TitleBarOpenFolderButton->SetDisabled(!bCanOpenFolderMenu);
        shell.TitleBarOpenFolderButton->SetContent(MakeTitleBarIcon(
            app.GetCoreIconBrush(
                ECoreIcon::OpenBuildDirectory,
                bCanOpenFolderMenu ? GetEditorTitleBarIconColor() : GetEditorTitleBarIconDisabledColor()),
            16.0f));
    }
}

void RefreshLocalizedEditorShell(
    ImApplication& app,
    FEditorShellWidgets& shell,
    const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    ApplyEditorThemeToShell(app, shell, workspaceController);
    app.SetApplicationTitle(EditorText("App.Title", "ImWidgetV4 Editor").Resolve());
    RebuildEditorTitleBar(app, shell, workspaceController);
    UpdateEditorTitleBarActions(app, shell, workspaceController);
    UpdateBuildOverviewPanel(shell, workspaceController);
    UpdateBuildDockActions(shell, app, workspaceController);
    if (workspaceController) {
        workspaceController->RefreshProjectTree();
    }
    if (shell.DetailsView) {
        shell.DetailsView->RebuildPreservingViewState();
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
        RegisterEditorThemePacks(app);
        SetEditorActiveTheme(app.GetActiveThemeName(), app.GetStyleSet());
        RegisterEditorDefaultStringTables();
        const std::filesystem::path defaultWorkspaceDirectory = GetDefaultEditorWorkspaceDirectory();
        std::error_code currentPathError;
        std::filesystem::current_path(defaultWorkspaceDirectory, currentPathError);

        WorkspaceStatePath_ = GetEditorWorkspaceStatePath();
        if (const std::string savedCulture = LoadSavedEditorCulture(WorkspaceStatePath_); !savedCulture.empty()) {
            app.SetCulture(savedCulture);
        }

        Shell_ = BuildEditorShell();
        WorkspaceController_ = std::make_shared<EditorWorkspaceController>(&BuildInitialDocumentRoot);
        Shell_.ShellHost->SetWorkspaceController(WorkspaceController_);
        if (Shell_.WidgetTreeHost) {
            Shell_.WidgetTreeHost->SetWorkspaceController(WorkspaceController_);
        }
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
        app.OnThemeChanged.AddLambda([this](ImApplication& application, const std::string&) {
            RefreshLocalizedEditorShell(application, Shell_, WorkspaceController_);
        });
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
                options.Title = EditorText("ProjectSettings.SelectAndroidSdkRoot", "Select Android SDK Root").Resolve();
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
                options.Title = EditorText("ProjectSettings.SelectAndroidNdkRoot", "Select Android NDK Root").Resolve();
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
        if (Shell_.BuildRegenerateCodeButton) {
            Shell_.BuildRegenerateCodeButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->RegenerateProjectCode();
                    }
                });
        }
        if (Shell_.BuildBuildButton) {
            Shell_.BuildBuildButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->BuildProject();
                    }
                });
        }
        if (Shell_.BuildRunButton) {
            Shell_.BuildRunButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        if (lockedWorkspace->IsRunTaskRunning()) {
                            lockedWorkspace->StopRunningProject();
                        } else {
                            lockedWorkspace->RunProject();
                        }
                    }
                });
        }
        if (Shell_.TitleBarConfigureButton) {
            Shell_.TitleBarConfigureButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->ConfigureProject();
                    }
                });
        }
        if (Shell_.TitleBarRegenerateCodeButton) {
            Shell_.TitleBarRegenerateCodeButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->RegenerateProjectCode();
                    }
                });
        }
        if (Shell_.TitleBarBuildButton) {
            Shell_.TitleBarBuildButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->BuildProject();
                    }
                });
        }
        if (Shell_.TitleBarRunButton) {
            Shell_.TitleBarRunButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        if (lockedWorkspace->IsRunTaskRunning()) {
                            lockedWorkspace->StopRunningProject();
                        } else {
                            lockedWorkspace->RunProject();
                        }
                    }
                });
        }
        if (Shell_.TitleBarOpenFolderButton) {
            BindPopupMenuButton(app, Shell_.TitleBarOpenFolderButton, [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)]() {
                return BuildOpenFolderMenuItems(weakWorkspace.lock());
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
        if (Shell_.BuildClearCacheButton) {
            Shell_.BuildClearCacheButton->OnClicked.AddLambda(
                [weakWorkspace = std::weak_ptr<EditorWorkspaceController>(WorkspaceController_)](ImButton&) {
                    if (auto lockedWorkspace = weakWorkspace.lock()) {
                        lockedWorkspace->ClearBuildCache();
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

    void OnShutdown(ImApplication& app) override
    {
        if (WorkspaceController_) {
            WorkspaceController_->SaveWorkspaceState(WorkspaceStatePath_);
        }
        SaveEditorCulture(WorkspaceStatePath_, app.GetCulture());
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
        if (BoundApplication_ != nullptr) {
            RefreshLocalizedEditorShell(*BoundApplication_, Shell_, WorkspaceController_);
        }
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
