#pragma once

#include "../toolchains/EnvironmentProbe.h"

#include <imwidgetv4/core/Application.h>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
class ImButton;
class ImComboBox;
class ImTextBlock;
class ImTextList;
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
    std::vector<std::string> BuildProfileNames;
    std::string ActiveBuildProfileName;
    FEnvironmentProbeReport ProbeReport;
    ImWidgetV4::FVector2 Position {240.0f, 120.0f};
    ImWidgetV4::FVector2 Size {620.0f, 460.0f};
    std::function<bool(const std::string& profileName)> OnConfirm;
    std::function<void()> OnCancel;
};

class ProjectSettingsDialog : public std::enable_shared_from_this<ProjectSettingsDialog> {
public:
    bool Open(ImWidgetV4::ImApplication& app, const FProjectSettingsDialogOptions& options);
    void Close(ImWidgetV4::ImApplication& app);
    bool IsOpen() const;

    std::shared_ptr<ImWidgetV4::ImWindow> GetWindow() const { return m_Window; }

private:
    void Reset();
    void SetErrorMessage(const std::string& message);

    FProjectSettingsDialogOptions m_Options;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Root;
    std::shared_ptr<ImWidgetV4::ImComboBox> m_ProfileComboBox;
    std::shared_ptr<ImWidgetV4::ImTextList> m_ProbeText;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_ErrorText;
    std::shared_ptr<ImWidgetV4::ImButton> m_ConfirmButton;
    std::shared_ptr<ImWidgetV4::ImButton> m_CancelButton;
    std::shared_ptr<ImWidgetV4::ImWindow> m_Window;
};

} // namespace ImWidgetV4Editor
