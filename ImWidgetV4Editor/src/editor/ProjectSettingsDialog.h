#pragma once

#include "../project/EditorProject.h"
#include "../toolchains/EnvironmentProbe.h"
#include "../toolchains/PlatformConfiguration.h"

#include <imwidgetv4/core/Application.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
class ImButton;
class ImComboBox;
class ImEditableText;
class ImSwitch;
class ImTextBlock;
class ImTextList;
class ImVerticalBox;
class ImWidget;
class ImWindow;
}

namespace ImWidgetV4Editor {

struct FProjectSettingsDialogOptions {
    std::string PopupTitle = "ProjectSettingsDialog";
    std::string HeadingText = "Project Settings";
    std::string ProjectName;
    std::string NamespaceName;
    std::string StartupDocument;
    FEditorApplicationSettings ApplicationSettings;
    std::vector<FEditorBuildProfile> BuildProfiles;
    std::string ActiveBuildProfileName;
    ImWidgetV4::FVector2 Position {240.0f, 120.0f};
    ImWidgetV4::FVector2 Size {680.0f, 700.0f};
    std::function<bool(const FEditorBuildProfile& profile, bool bMakeActive, const FEditorApplicationSettings& applicationSettings)> OnConfirm;
    std::function<void()> OnCancel;
};

class ProjectSettingsDialog : public std::enable_shared_from_this<ProjectSettingsDialog> {
public:
    bool Open(ImWidgetV4::ImApplication& app, const FProjectSettingsDialogOptions& options);
    void Close(ImWidgetV4::ImApplication& app);
    bool IsOpen() const;

    std::shared_ptr<ImWidgetV4::ImWindow> GetWindow() const { return m_Window; }

private:
    FEditorBuildProfile* GetSelectedProfile();
    const FEditorBuildProfile* GetSelectedProfile() const;
    void PopulateProfileComboBox();
    void UpdateProfileEditorsFromSelection();
    bool ApplyEditorValuesToSelection(std::string* outError = nullptr);
    bool ApplyApplicationEditorValues(std::string* outError = nullptr);
    void RefreshProbeReport();
    void RefreshAndroidEditorVisibility();
    void RefreshApplicationEditorVisibility();
    void Reset();
    void SetErrorMessage(const std::string& message);

    FProjectSettingsDialogOptions m_Options;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Root;
    std::shared_ptr<ImWidgetV4::ImComboBox> m_ProfileComboBox;
    std::shared_ptr<ImWidgetV4::ImComboBox> m_WindowsGeneratorComboBox;
    std::shared_ptr<ImWidgetV4::ImComboBox> m_AndroidAbiComboBox;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_AndroidApiLevelEditor;
    std::shared_ptr<ImWidgetV4::ImComboBox> m_AndroidStlComboBox;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_AndroidSdkRootEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_AndroidNdkRootEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_ApplicationTitleEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_InitialWidthEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_InitialHeightEditor;
    std::shared_ptr<ImWidgetV4::ImSwitch> m_EnableIniSettingsSwitch;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_IniSettingsPathEditor;
    std::shared_ptr<ImWidgetV4::ImSwitch> m_UseCustomHostChromeSwitch;
    std::shared_ptr<ImWidgetV4::ImSwitch> m_UseTitleBarSwitch;
    std::shared_ptr<ImWidgetV4::ImSwitch> m_ShowSystemButtonsSwitch;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_DefaultThemeEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_DefaultCultureEditor;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> m_IniSettingsGroup;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> m_TitleBarSettingsGroup;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> m_WindowsSettingsGroup;
    std::shared_ptr<ImWidgetV4::ImVerticalBox> m_AndroidSettingsGroup;
    std::shared_ptr<ImWidgetV4::ImTextList> m_ProbeText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_ErrorText;
    std::shared_ptr<ImWidgetV4::ImButton> m_ReprobeButton;
    std::shared_ptr<ImWidgetV4::ImButton> m_ConfirmButton;
    std::shared_ptr<ImWidgetV4::ImButton> m_CancelButton;
    std::shared_ptr<ImWidgetV4::ImWindow> m_Window;
};

} // namespace ImWidgetV4Editor
