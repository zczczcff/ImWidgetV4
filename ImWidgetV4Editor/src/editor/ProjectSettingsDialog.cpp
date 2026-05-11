#include "ProjectSettingsDialog.h"

#include "EditorLocalization.h"
#include "../inspector/PropertyEditorWidgets.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor::PropertyEditorWidgets;

namespace {

FButtonStyle MakeDialogButtonStyle(bool bPrimary)
{
    return bPrimary ? FButtonStyle::CreatePrimary() : FButtonStyle();
}

FVector2 MaxSize(const FVector2& left, const FVector2& right)
{
    return FVector2(
        std::max(left.X, right.X),
        std::max(left.Y, right.Y));
}

std::vector<std::string> BuildProbeLines(const FEnvironmentProbeReport& report)
{
    std::vector<std::string> lines;
    lines.push_back(EditorText("ProjectSettings.ProbeTarget", "Target").Resolve() + ": " + GetTargetPlatformDisplayName(report.TargetPlatform));
    lines.push_back(EditorText("ProjectSettings.ProbeReady", "Ready").Resolve() + ": " +
        (report.bReady
            ? EditorText("Common.Yes", "Yes").Resolve()
            : EditorText("Common.No", "No").Resolve()));
    lines.push_back("");
    for (const FEnvironmentProbeItem& item : report.Items) {
        lines.push_back(item.Label + " [" + ToDisplayString(item.Status) + "]");
        lines.push_back("  " + item.Details);
    }
    return lines;
}

std::vector<std::string> BuildProfileNames(const std::vector<FEditorBuildProfile>& profiles)
{
    std::vector<std::string> names;
    names.reserve(profiles.size());
    for (const FEditorBuildProfile& profile : profiles) {
        names.push_back(profile.Name);
    }
    return names;
}

std::vector<std::string> GetAndroidAbiOptions()
{
    return {"arm64-v8a", "armeabi-v7a", "x86_64", "x86"};
}

std::vector<std::string> GetAndroidStlOptions()
{
    return {"c++_shared", "c++_static"};
}

std::vector<std::string> GetWindowsGeneratorOptions()
{
    return {
        "",
        "Ninja",
        "Visual Studio 17 2022"
    };
}

std::string GetWindowsGeneratorDisplayLabel(const std::string& generator)
{
    return generator.empty() ? EditorText("Common.Default", "Default").Resolve() : generator;
}

std::shared_ptr<ImHorizontalBox> MakePathOverrideEditorRow(
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

} // namespace

bool ProjectSettingsDialog::Open(ImApplication& app, const FProjectSettingsDialogOptions& options)
{
    Close(app);
    m_Options = options;

    auto title = std::make_shared<ImTextBlock>();
    title->SetText(m_Options.HeadingText);
    title->SetWrapText(false);
    title->SetFontSize(16.0f);
    title->SetTextColor(FColor::FromBytes(238, 242, 247));

    auto projectNameField = MakeInspectorReadOnlyField(m_Options.ProjectName);
    auto namespaceField = MakeInspectorReadOnlyField(m_Options.NamespaceName);
    auto startupField = MakeInspectorReadOnlyField(m_Options.StartupDocument);

    auto profileComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*profileComboBox);
    profileComboBox->SetItems(BuildProfileNames(m_Options.BuildProfiles));

    auto windowsGeneratorComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*windowsGeneratorComboBox);
    std::vector<std::string> windowsGeneratorLabels;
    for (const std::string& generator : GetWindowsGeneratorOptions()) {
        windowsGeneratorLabels.push_back(GetWindowsGeneratorDisplayLabel(generator));
    }
    windowsGeneratorComboBox->SetItems(windowsGeneratorLabels);

    auto androidAbiComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*androidAbiComboBox);
    androidAbiComboBox->SetItems(GetAndroidAbiOptions());

    auto androidApiLevelEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*androidApiLevelEditor, false);

    auto androidStlComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*androidStlComboBox);
    androidStlComboBox->SetItems(GetAndroidStlOptions());

    auto androidSdkRootEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*androidSdkRootEditor, false);
    androidSdkRootEditor->SetHintText(EditorText("Build.OverrideAndroidSdkRoot", "Override Android SDK root"));

    auto androidSdkBrowseButton = std::make_shared<ImButton>();
    androidSdkBrowseButton->SetStyle(MakeDialogButtonStyle(false));
    androidSdkBrowseButton->SetText(EditorText("Build.Browse", "Browse"));

    auto androidSdkClearButton = std::make_shared<ImButton>();
    androidSdkClearButton->SetStyle(MakeDialogButtonStyle(false));
    androidSdkClearButton->SetText(EditorText("Build.Clear", "Clear"));

    auto androidNdkRootEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*androidNdkRootEditor, false);
    androidNdkRootEditor->SetHintText(EditorText("Build.OverrideAndroidNdkRoot", "Override Android NDK root"));

    auto androidNdkBrowseButton = std::make_shared<ImButton>();
    androidNdkBrowseButton->SetStyle(MakeDialogButtonStyle(false));
    androidNdkBrowseButton->SetText(EditorText("Build.Browse", "Browse"));

    auto androidNdkClearButton = std::make_shared<ImButton>();
    androidNdkClearButton->SetStyle(MakeDialogButtonStyle(false));
    androidNdkClearButton->SetText(EditorText("Build.Clear", "Clear"));

    auto windowsSettingsGroup = std::make_shared<ImVerticalBox>();
    windowsSettingsGroup->SetSpacing(8.0f);
    windowsSettingsGroup->AddChild(MakeInspectorVerticalPropertyRow(EditorText("ProjectSettings.WindowsGenerator", "Windows Generator").Resolve(), windowsGeneratorComboBox), FMargin(0.0f));

    auto androidSettingsGroup = std::make_shared<ImVerticalBox>();
    androidSettingsGroup->SetSpacing(8.0f);
    androidSettingsGroup->AddChild(MakeInspectorVerticalPropertyRow(EditorText("ProjectSettings.AndroidAbi", "Android ABI").Resolve(), androidAbiComboBox), FMargin(0.0f));
    androidSettingsGroup->AddChild(MakeInspectorVerticalPropertyRow(EditorText("ProjectSettings.AndroidApiLevel", "Android API Level").Resolve(), androidApiLevelEditor), FMargin(0.0f));
    androidSettingsGroup->AddChild(MakeInspectorVerticalPropertyRow(EditorText("ProjectSettings.AndroidStl", "Android STL").Resolve(), androidStlComboBox), FMargin(0.0f));
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow(
            EditorText("Build.AndroidSdkRoot", "Android SDK Root").Resolve(),
            MakePathOverrideEditorRow(androidSdkRootEditor, androidSdkBrowseButton, androidSdkClearButton)),
        FMargin(0.0f));
    androidSettingsGroup->AddChild(
        MakeInspectorVerticalPropertyRow(
            EditorText("Build.AndroidNdkRoot", "Android NDK Root").Resolve(),
            MakePathOverrideEditorRow(androidNdkRootEditor, androidNdkBrowseButton, androidNdkClearButton)),
        FMargin(0.0f));

    auto probeText = std::make_shared<ImTextList>();
    FTextListStyle probeStyle = probeText->GetStyle();
    probeStyle.BackgroundColor = FColor::FromBytes(18, 23, 29);
    probeStyle.BorderColor = FColor::FromBytes(16, 19, 23);
    probeStyle.FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    probeStyle.TextColor = FColor::FromBytes(196, 205, 217);
    probeStyle.SelectionBackgroundColor = FColor::FromBytes(72, 104, 146, 148);
    probeStyle.Padding = FMargin(10.0f);
    probeStyle.MinDesiredSize = FVector2(0.0f, 200.0f);
    probeStyle.CornerRadius = 5.0f;
    probeStyle.BorderThickness = 1.0f;
    probeStyle.FontSize = 13.0f;
    probeStyle.LineSpacing = 1.1f;
    probeText->SetStyle(probeStyle);
    probeText->SetItems(std::vector<std::string>{});

    auto errorText = std::make_shared<ImTextBlock>();
    errorText->SetText("");
    errorText->SetWrapText(false);
    errorText->SetFontSize(12.0f);
    errorText->SetTextColor(FColor::FromBytes(239, 103, 103));

    auto confirmButton = std::make_shared<ImButton>();
    confirmButton->SetStyle(MakeDialogButtonStyle(true));
    confirmButton->SetText(EditorText("Build.Apply", "Apply"));

    auto reprobeButton = std::make_shared<ImButton>();
    reprobeButton->SetStyle(MakeDialogButtonStyle(false));
    reprobeButton->SetText(EditorText("Build.ReProbe", "Re-Probe"));

    auto cancelButton = std::make_shared<ImButton>();
    cancelButton->SetStyle(MakeDialogButtonStyle(false));
    cancelButton->SetText(EditorText("Common.Close", "Close"));

    auto fields = std::make_shared<ImVerticalBox>();
    fields->SetSpacing(8.0f);
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("TitleBar.Project", "Project").Resolve(), projectNameField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.Namespace", "Namespace").Resolve(), namespaceField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.StartupUI", "Startup UI").Resolve(), startupField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("ProjectSettings.ActiveBuildProfile", "Active Build Profile").Resolve(), profileComboBox), FMargin(0.0f));
    fields->AddChild(windowsSettingsGroup, FMargin(0.0f));
    fields->AddChild(androidSettingsGroup, FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("ProjectSettings.EnvironmentProbe", "Environment Probe").Resolve(), probeText), FMargin(0.0f));
    fields->AddChild(errorText, FMargin(2.0f, 0.0f, 2.0f, 0.0f));

    auto buttonRow = std::make_shared<ImHorizontalBox>();
    buttonRow->SetSpacing(8.0f);
    buttonRow->AddChildFill(MakeInspectorFlexibleSpacer(), 1.0f, FMargin(0.0f));
    buttonRow->AddChild(reprobeButton);
    buttonRow->AddChild(confirmButton);
    buttonRow->AddChild(cancelButton);

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(10.0f);
    root->AddChild(title, FMargin(12.0f, 12.0f, 12.0f, 0.0f));
    root->AddChild(fields, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    root->AddChild(buttonRow, FMargin(12.0f, 0.0f, 12.0f, 12.0f));
    const FVector2 popupContentMinSize = root->GetMinSize();

    m_Root = root;
    m_ProfileComboBox = profileComboBox;
    m_WindowsGeneratorComboBox = windowsGeneratorComboBox;
    m_AndroidAbiComboBox = androidAbiComboBox;
    m_AndroidApiLevelEditor = androidApiLevelEditor;
    m_AndroidStlComboBox = androidStlComboBox;
    m_AndroidSdkRootEditor = androidSdkRootEditor;
    m_AndroidNdkRootEditor = androidNdkRootEditor;
    m_WindowsSettingsGroup = windowsSettingsGroup;
    m_AndroidSettingsGroup = androidSettingsGroup;
    m_ProbeText = probeText;
    m_ErrorText = errorText;
    m_ReprobeButton = reprobeButton;
    m_ConfirmButton = confirmButton;
    m_CancelButton = cancelButton;

    PopulateProfileComboBox();
    UpdateProfileEditorsFromSelection();
    RefreshAndroidEditorVisibility();
    RefreshProbeReport();

    const std::weak_ptr<ProjectSettingsDialog> weakThis = weak_from_this();
    auto requestConfirm = [weakThis, &app]() {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            std::string error;
            if (!self->ApplyEditorValuesToSelection(&error)) {
                self->SetErrorMessage(error);
                return;
            }

            FEditorBuildProfile* selectedProfile = self->GetSelectedProfile();
            if (selectedProfile == nullptr) {
                self->SetErrorMessage(EditorText("ProjectSettings.SelectBuildProfile", "Select a build profile.").Resolve());
                return;
            }

            const std::function<bool(const FEditorBuildProfile&, bool)> onConfirm = self->m_Options.OnConfirm;
            if (onConfirm && !onConfirm(*selectedProfile, true)) {
                return;
            }

            self->Close(app);
        }
    };

    auto requestCancel = [weakThis, &app]() {
        if (auto self = weakThis.lock()) {
            const std::function<void()> onCancel = self->m_Options.OnCancel;
            self->Close(app);
            if (onCancel) {
                onCancel();
            }
        }
    };

    profileComboBox->OnSelectionChanged.AddLambda([weakThis](ImComboBox&, int) {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            self->UpdateProfileEditorsFromSelection();
            self->RefreshAndroidEditorVisibility();
            self->RefreshProbeReport();
        }
    });

    androidSdkBrowseButton->OnClicked.AddLambda([weakThis, &app](ImButton&) {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");

            FOpenFolderDialogOptions options;
            options.Title = EditorText("ProjectSettings.SelectAndroidSdkRoot", "Select Android SDK Root").Resolve();
            if (self->m_AndroidSdkRootEditor && !self->m_AndroidSdkRootEditor->GetText().empty()) {
                options.InitialDirectory = std::filesystem::path(self->m_AndroidSdkRootEditor->GetText());
            } else if (const FEditorBuildProfile* selectedProfile = self->GetSelectedProfile()) {
                options.InitialDirectory = ResolveAndroidSdkRootForProfile(*selectedProfile);
            }

            const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
            if (dialogResult.IsAccepted()) {
                self->m_AndroidSdkRootEditor->SetText(dialogResult.Path.string());
                std::string error;
                if (!self->ApplyEditorValuesToSelection(&error)) {
                    self->SetErrorMessage(error);
                    return;
                }
                self->RefreshProbeReport();
                return;
            }

            if (dialogResult.Code == EPathDialogResultCode::Unsupported) {
                self->SetErrorMessage(EditorText("ProjectSettings.FolderSelectionUnsupported", "Folder selection is unsupported by the active platform backend.").Resolve());
            } else if (dialogResult.Code == EPathDialogResultCode::Error) {
                self->SetErrorMessage(EditorText("ProjectSettings.AndroidSdkRootSelectionFailed", "Android SDK root selection failed").Resolve() + ": " + dialogResult.ErrorMessage);
            }
        }
    });

    androidSdkClearButton->OnClicked.AddLambda([weakThis](ImButton&) {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            if (self->m_AndroidSdkRootEditor) {
                self->m_AndroidSdkRootEditor->SetText("");
            }
            std::string error;
            if (!self->ApplyEditorValuesToSelection(&error)) {
                self->SetErrorMessage(error);
                return;
            }
            self->RefreshProbeReport();
        }
    });

    androidNdkBrowseButton->OnClicked.AddLambda([weakThis, &app](ImButton&) {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");

            FOpenFolderDialogOptions options;
            options.Title = EditorText("ProjectSettings.SelectAndroidNdkRoot", "Select Android NDK Root").Resolve();
            if (self->m_AndroidNdkRootEditor && !self->m_AndroidNdkRootEditor->GetText().empty()) {
                options.InitialDirectory = std::filesystem::path(self->m_AndroidNdkRootEditor->GetText());
            } else if (const FEditorBuildProfile* selectedProfile = self->GetSelectedProfile()) {
                options.InitialDirectory =
                    ResolveAndroidNdkRootForProfile(*selectedProfile, ResolveAndroidSdkRootForProfile(*selectedProfile));
            }

            const FPathDialogResult dialogResult = app.OpenFolderDialog(options);
            if (dialogResult.IsAccepted()) {
                self->m_AndroidNdkRootEditor->SetText(dialogResult.Path.string());
                std::string error;
                if (!self->ApplyEditorValuesToSelection(&error)) {
                    self->SetErrorMessage(error);
                    return;
                }
                self->RefreshProbeReport();
                return;
            }

            if (dialogResult.Code == EPathDialogResultCode::Unsupported) {
                self->SetErrorMessage(EditorText("ProjectSettings.FolderSelectionUnsupported", "Folder selection is unsupported by the active platform backend.").Resolve());
            } else if (dialogResult.Code == EPathDialogResultCode::Error) {
                self->SetErrorMessage(EditorText("ProjectSettings.AndroidNdkRootSelectionFailed", "Android NDK root selection failed").Resolve() + ": " + dialogResult.ErrorMessage);
            }
        }
    });

    androidNdkClearButton->OnClicked.AddLambda([weakThis](ImButton&) {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            if (self->m_AndroidNdkRootEditor) {
                self->m_AndroidNdkRootEditor->SetText("");
            }
            std::string error;
            if (!self->ApplyEditorValuesToSelection(&error)) {
                self->SetErrorMessage(error);
                return;
            }
            self->RefreshProbeReport();
        }
    });

    reprobeButton->OnClicked.AddLambda([weakThis](ImButton&) {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            std::string error;
            if (!self->ApplyEditorValuesToSelection(&error)) {
                self->SetErrorMessage(error);
                return;
            }
            self->RefreshProbeReport();
        }
    });

    confirmButton->OnClicked.AddLambda([requestConfirm](ImButton&) {
        requestConfirm();
    });
    cancelButton->OnClicked.AddLambda([requestCancel](ImButton&) {
        requestCancel();
    });

    FPopupOptions popupOptions;
    popupOptions.Title = m_Options.PopupTitle;
    popupOptions.Position = m_Options.Position;
    popupOptions.Size = MaxSize(m_Options.Size, popupContentMinSize);
    popupOptions.RootWidget = root;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_Window = app.GetWindowManager().CreatePopup(popupOptions);
    app.SetKeyboardFocus(profileComboBox);
    return static_cast<bool>(m_Window);
}

void ProjectSettingsDialog::Close(ImApplication& app)
{
    if (m_Window) {
        app.GetWindowManager().CloseWindow(m_Window);
    }
    Reset();
}

bool ProjectSettingsDialog::IsOpen() const
{
    return m_Window && m_Window->IsOpen();
}

void ProjectSettingsDialog::Reset()
{
    m_Window.reset();
    m_Root.reset();
    m_ProfileComboBox.reset();
    m_WindowsGeneratorComboBox.reset();
    m_AndroidAbiComboBox.reset();
    m_AndroidApiLevelEditor.reset();
    m_AndroidStlComboBox.reset();
    m_AndroidSdkRootEditor.reset();
    m_AndroidNdkRootEditor.reset();
    m_WindowsSettingsGroup.reset();
    m_AndroidSettingsGroup.reset();
    m_ProbeText.reset();
    m_ErrorText.reset();
    m_ReprobeButton.reset();
    m_ConfirmButton.reset();
    m_CancelButton.reset();
    m_Options = FProjectSettingsDialogOptions();
}

void ProjectSettingsDialog::SetErrorMessage(const std::string& message)
{
    if (m_ErrorText) {
        m_ErrorText->SetText(message);
    }
}

FEditorBuildProfile* ProjectSettingsDialog::GetSelectedProfile()
{
    if (!m_ProfileComboBox || !m_ProfileComboBox->HasSelection()) {
        return nullptr;
    }

    const std::string profileName = m_ProfileComboBox->GetSelectedText();
    return FindBuildProfileByName(m_Options.BuildProfiles, profileName);
}

const FEditorBuildProfile* ProjectSettingsDialog::GetSelectedProfile() const
{
    if (!m_ProfileComboBox || !m_ProfileComboBox->HasSelection()) {
        return nullptr;
    }

    const std::string profileName = m_ProfileComboBox->GetSelectedText();
    return FindBuildProfileByName(m_Options.BuildProfiles, profileName);
}

void ProjectSettingsDialog::PopulateProfileComboBox()
{
    if (!m_ProfileComboBox) {
        return;
    }

    m_ProfileComboBox->SetItems(BuildProfileNames(m_Options.BuildProfiles));
    for (int index = 0; index < static_cast<int>(m_Options.BuildProfiles.size()); ++index) {
        if (m_Options.BuildProfiles[static_cast<std::size_t>(index)].Name == m_Options.ActiveBuildProfileName) {
            m_ProfileComboBox->SetSelectedIndex(index);
            return;
        }
    }

    if (!m_Options.BuildProfiles.empty()) {
        m_ProfileComboBox->SetSelectedIndex(0);
    }
}

void ProjectSettingsDialog::UpdateProfileEditorsFromSelection()
{
    const FEditorBuildProfile* selectedProfile = GetSelectedProfile();
    if (selectedProfile == nullptr) {
        return;
    }

    if (m_AndroidAbiComboBox) {
        const auto abiOptions = GetAndroidAbiOptions();
        for (int index = 0; index < static_cast<int>(abiOptions.size()); ++index) {
            if (abiOptions[static_cast<std::size_t>(index)] == selectedProfile->AndroidSettings.Abi) {
                m_AndroidAbiComboBox->SetSelectedIndex(index);
                break;
            }
        }
    }

    if (m_AndroidApiLevelEditor) {
        m_AndroidApiLevelEditor->SetText(std::to_string(selectedProfile->AndroidSettings.ApiLevel));
    }

    if (m_WindowsGeneratorComboBox) {
        const auto generatorOptions = GetWindowsGeneratorOptions();
        for (int index = 0; index < static_cast<int>(generatorOptions.size()); ++index) {
            if (generatorOptions[static_cast<std::size_t>(index)] == selectedProfile->Generator) {
                m_WindowsGeneratorComboBox->SetSelectedIndex(index);
                break;
            }
        }
    }

    if (m_AndroidStlComboBox) {
        const auto stlOptions = GetAndroidStlOptions();
        for (int index = 0; index < static_cast<int>(stlOptions.size()); ++index) {
            if (stlOptions[static_cast<std::size_t>(index)] == selectedProfile->AndroidSettings.Stl) {
                m_AndroidStlComboBox->SetSelectedIndex(index);
                break;
            }
        }
    }

    if (m_AndroidSdkRootEditor) {
        m_AndroidSdkRootEditor->SetText(selectedProfile->AndroidSettings.SdkRootOverride.string());
    }

    if (m_AndroidNdkRootEditor) {
        m_AndroidNdkRootEditor->SetText(selectedProfile->AndroidSettings.NdkRootOverride.string());
    }
}

bool ProjectSettingsDialog::ApplyEditorValuesToSelection(std::string* outError)
{
    FEditorBuildProfile* selectedProfile = GetSelectedProfile();
    if (selectedProfile == nullptr) {
        if (outError) {
            *outError = EditorText("ProjectSettings.SelectBuildProfile", "Select a build profile.").Resolve();
        }
        return false;
    }

    if (selectedProfile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop) {
        if (!m_WindowsGeneratorComboBox || !m_WindowsGeneratorComboBox->HasSelection()) {
            if (outError) {
                *outError = EditorText("ProjectSettings.SelectWindowsGenerator", "Select a Windows generator.").Resolve();
            }
            return false;
        }

        const int selectedIndex = m_WindowsGeneratorComboBox->GetSelectedIndex();
        const auto generatorOptions = GetWindowsGeneratorOptions();
        if (selectedIndex < 0 || selectedIndex >= static_cast<int>(generatorOptions.size())) {
            if (outError) {
                *outError = EditorText("ProjectSettings.SelectValidWindowsGenerator", "Select a valid Windows generator.").Resolve();
            }
            return false;
        }

        selectedProfile->Generator = generatorOptions[static_cast<std::size_t>(selectedIndex)];
        return true;
    }

    if (selectedProfile->TargetPlatform != EEditorTargetPlatform::Android) {
        return true;
    }

    if (!m_AndroidAbiComboBox || !m_AndroidAbiComboBox->HasSelection()) {
        if (outError) {
            *outError = EditorText("ProjectSettings.SelectAndroidAbi", "Select an Android ABI.").Resolve();
        }
        return false;
    }

    if (!m_AndroidStlComboBox || !m_AndroidStlComboBox->HasSelection()) {
        if (outError) {
            *outError = EditorText("ProjectSettings.SelectAndroidStl", "Select an Android STL.").Resolve();
        }
        return false;
    }

    selectedProfile->AndroidSettings.Abi = m_AndroidAbiComboBox->GetSelectedText();
    selectedProfile->AndroidSettings.Stl = m_AndroidStlComboBox->GetSelectedText();

    int apiLevel = 0;
    try {
        apiLevel = m_AndroidApiLevelEditor ? std::stoi(m_AndroidApiLevelEditor->GetText()) : 0;
    } catch (...) {
        apiLevel = 0;
    }

    if (apiLevel < 21) {
        if (outError) {
            *outError = EditorText("ProjectSettings.AndroidApiLevelTooLow", "Android API Level must be 21 or higher.").Resolve();
        }
        return false;
    }

    selectedProfile->AndroidSettings.ApiLevel = apiLevel;
    selectedProfile->AndroidSettings.SdkRootOverride =
        m_AndroidSdkRootEditor ? std::filesystem::path(m_AndroidSdkRootEditor->GetText()).lexically_normal() : std::filesystem::path();
    selectedProfile->AndroidSettings.NdkRootOverride =
        m_AndroidNdkRootEditor ? std::filesystem::path(m_AndroidNdkRootEditor->GetText()).lexically_normal() : std::filesystem::path();
    return true;
}

void ProjectSettingsDialog::RefreshProbeReport()
{
    const FEditorBuildProfile* selectedProfile = GetSelectedProfile();
    if (selectedProfile == nullptr || !m_ProbeText) {
        return;
    }

    m_ProbeText->SetItems(BuildProbeLines(EnvironmentProbe::Probe(*selectedProfile)));
}

void ProjectSettingsDialog::RefreshAndroidEditorVisibility()
{
    const FEditorBuildProfile* selectedProfile = GetSelectedProfile();
    const bool bShowWindowsSettings =
        selectedProfile != nullptr && selectedProfile->TargetPlatform == EEditorTargetPlatform::WindowsDesktop;
    const bool bShowAndroidSettings =
        selectedProfile != nullptr && selectedProfile->TargetPlatform == EEditorTargetPlatform::Android;
    if (m_WindowsSettingsGroup) {
        m_WindowsSettingsGroup->SetVisible(bShowWindowsSettings);
    }
    if (m_AndroidSettingsGroup) {
        m_AndroidSettingsGroup->SetVisible(bShowAndroidSettings);
    }
}

} // namespace ImWidgetV4Editor
