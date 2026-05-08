#pragma once

#include <imwidgetv4/core/Application.h>
#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {
class ImButton;
class ImComboBox;
class ImEditableText;
class ImTextBlock;
class ImWidget;
class ImWindow;
}

namespace ImWidgetV4Editor {

struct FCreateAppProjectOptions {
    std::string ProjectName;
    std::string NamespaceName;
    std::string StartupDocumentName;
    std::string TemplateName = "Blank App";
};

struct FNewAppProjectDialogOptions {
    std::string PopupTitle = "CreateAppProjectDialog";
    std::string HeadingText = "Create App Project";
    std::filesystem::path ParentDirectory;
    FCreateAppProjectOptions InitialOptions;
    std::vector<std::string> TemplateOptions {"Blank App"};
    int InitialTemplateIndex = 0;
    std::string ConfirmText = "Create";
    std::string CancelText = "Cancel";
    ImWidgetV4::FVector2 Position {220.0f, 120.0f};
    ImWidgetV4::FVector2 Size {520.0f, 316.0f};
    std::function<bool(const FCreateAppProjectOptions& options)> OnConfirm;
    std::function<void()> OnCancel;
};

class NewAppProjectDialog : public std::enable_shared_from_this<NewAppProjectDialog> {
public:
    bool Open(ImWidgetV4::ImApplication& app, const FNewAppProjectDialogOptions& options);
    void Close(ImWidgetV4::ImApplication& app);
    bool IsOpen() const;

    std::shared_ptr<ImWidgetV4::ImWindow> GetWindow() const { return m_Window; }

private:
    void Reset();
    void SetErrorMessage(const std::string& message);
    FCreateAppProjectOptions BuildCurrentOptions() const;

    FNewAppProjectDialogOptions m_Options;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Root;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_ProjectNameEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_NamespaceEditor;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_StartupDocumentEditor;
    std::shared_ptr<ImWidgetV4::ImComboBox> m_TemplateComboBox;
    std::shared_ptr<ImWidgetV4::ImTextBlock> m_ErrorText;
    std::shared_ptr<ImWidgetV4::ImButton> m_ConfirmButton;
    std::shared_ptr<ImWidgetV4::ImButton> m_CancelButton;
    std::shared_ptr<ImWidgetV4::ImWindow> m_Window;
    bool m_bNamespaceEdited = false;
    bool m_bStartupDocumentEdited = false;
    bool m_bUpdatingDerivedFields = false;
};

} // namespace ImWidgetV4Editor
