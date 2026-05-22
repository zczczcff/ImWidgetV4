#include "NewAppProjectDialog.h"
#include "EditorTheme.h"
#include "EditorLocalization.h"
#include "../inspector/PropertyEditorWidgets.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/ComboBox.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>
#include <cctype>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor::PropertyEditorWidgets;

namespace {

FVector2 MaxSize(const FVector2& left, const FVector2& right)
{
    return FVector2(
        std::max(left.X, right.X),
        std::max(left.Y, right.Y));
}

bool IsIdentifierStartChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalpha(value) != 0 || c == '_';
}

bool IsIdentifierContinueChar(char c)
{
    const unsigned char value = static_cast<unsigned char>(c);
    return std::isalnum(value) != 0 || c == '_';
}

std::string TrimWhitespaceCopy(const std::string& text)
{
    const std::size_t begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return std::string();
    }

    const std::size_t end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string NormalizeProjectIdentifier(const std::string& rawText, const std::string& fallback)
{
    const std::string source = TrimWhitespaceCopy(rawText);
    const std::string fallbackSource = TrimWhitespaceCopy(fallback).empty()
        ? std::string("AppProject")
        : TrimWhitespaceCopy(fallback);

    auto sanitize = [](const std::string& text) {
        std::string result;
        result.reserve(text.size());
        for (char c : text) {
            if (IsIdentifierContinueChar(c)) {
                result.push_back(c);
            } else if (result.empty() || result.back() != '_') {
                result.push_back('_');
            }
        }

        while (!result.empty() && result.back() == '_') {
            result.pop_back();
        }

        return result;
    };

    std::string normalized = sanitize(source);
    if (normalized.empty()) {
        normalized = sanitize(fallbackSource);
    }
    if (normalized.empty()) {
        normalized = "AppProject";
    }
    if (!IsIdentifierStartChar(normalized.front())) {
        normalized.insert(normalized.begin(), '_');
    }
    return normalized;
}

std::string NormalizeStartupDocumentName(const std::string& rawText)
{
    const std::string trimmed = TrimWhitespaceCopy(rawText);
    if (trimmed.empty()) {
        return "MainView";
    }

    return std::filesystem::path(trimmed).stem().string();
}

} // namespace

bool NewAppProjectDialog::Open(ImApplication& app, const FNewAppProjectDialogOptions& options)
{
    Close(app);
    m_Options = options;

    auto title = std::make_shared<ImTextBlock>();
    title->SetText(m_Options.HeadingText);
    title->SetWrapText(false);
    title->SetFontSize(16.0f);
    title->SetTextColor(GetEditorPanelTitleColor());

    auto parentDirectoryField = MakeInspectorReadOnlyField(m_Options.ParentDirectory.string());

    auto projectNameEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*projectNameEditor, false);
    projectNameEditor->SetText(m_Options.InitialOptions.ProjectName);
    projectNameEditor->SetHintText("MyApp");

    auto namespaceEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*namespaceEditor, false);
    namespaceEditor->SetText(m_Options.InitialOptions.NamespaceName);
    namespaceEditor->SetHintText("MyApp");

    auto startupDocumentEditor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*startupDocumentEditor, false);
    startupDocumentEditor->SetText(m_Options.InitialOptions.StartupDocumentName);
    startupDocumentEditor->SetHintText("Main");

    auto templateComboBox = std::make_shared<ImComboBox>();
    ApplyInspectorComboBoxStyle(*templateComboBox);
    templateComboBox->SetItems(m_Options.TemplateOptions);
    if (!m_Options.TemplateOptions.empty()) {
        templateComboBox->SetSelectedIndex(std::clamp(
            m_Options.InitialTemplateIndex,
            0,
            static_cast<int>(m_Options.TemplateOptions.size()) - 1));
    }

    auto errorText = std::make_shared<ImTextBlock>();
    errorText->SetText("");
    errorText->SetWrapText(false);
    errorText->SetFontSize(12.0f);
    errorText->SetTextColor(GetEditorDangerColor());

    auto confirmButton = std::make_shared<ImButton>();
    confirmButton->SetStyle(MakeEditorDialogButtonStyle(true));
    confirmButton->SetText(m_Options.ConfirmText);

    auto cancelButton = std::make_shared<ImButton>();
    cancelButton->SetStyle(MakeEditorDialogButtonStyle(false));
    cancelButton->SetText(m_Options.CancelText);

    auto fields = std::make_shared<ImVerticalBox>();
    fields->SetSpacing(8.0f);
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.ParentDirectory", "Parent Directory").Resolve(), parentDirectoryField), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.ProjectName", "Project Name").Resolve(), projectNameEditor), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.Namespace", "Namespace").Resolve(), namespaceEditor), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.StartupUI", "Startup UI").Resolve(), startupDocumentEditor), FMargin(0.0f));
    fields->AddChild(MakeInspectorVerticalPropertyRow(EditorText("NewProject.Template", "Template").Resolve(), templateComboBox), FMargin(0.0f));
    fields->AddChild(errorText, FMargin(2.0f, 0.0f, 2.0f, 0.0f));

    auto buttonRow = std::make_shared<ImHorizontalBox>();
    buttonRow->SetSpacing(8.0f);
    buttonRow->AddChildFill(MakeInspectorFlexibleSpacer(), 1.0f, FMargin(0.0f));
    buttonRow->AddChild(confirmButton);
    buttonRow->AddChild(cancelButton);

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(10.0f);
    root->AddChild(title, FMargin(12.0f, 12.0f, 12.0f, 0.0f));
    root->AddChild(fields, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    root->AddChild(buttonRow, FMargin(12.0f, 0.0f, 12.0f, 12.0f));
    const FVector2 popupContentMinSize = root->GetMinSize();

    m_Root = root;
    m_ProjectNameEditor = projectNameEditor;
    m_NamespaceEditor = namespaceEditor;
    m_StartupDocumentEditor = startupDocumentEditor;
    m_TemplateComboBox = templateComboBox;
    m_ErrorText = errorText;
    m_ConfirmButton = confirmButton;
    m_CancelButton = cancelButton;
    m_bNamespaceEdited = false;
    m_bStartupDocumentEdited = false;

    const std::weak_ptr<NewAppProjectDialog> weakThis = weak_from_this();
    auto requestConfirm = [weakThis, &app]() {
        if (auto self = weakThis.lock()) {
            self->SetErrorMessage("");
            const std::function<bool(const FCreateAppProjectOptions&)> onConfirm = self->m_Options.OnConfirm;
            if (onConfirm && !onConfirm(self->BuildCurrentOptions())) {
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

    projectNameEditor->OnTextChanged.AddLambda([weakThis](ImEditableText&, const std::string& text) {
        if (auto self = weakThis.lock()) {
            self->m_bUpdatingDerivedFields = true;
            if (!self->m_bNamespaceEdited && self->m_NamespaceEditor) {
                self->m_NamespaceEditor->SetText(NormalizeProjectIdentifier(text, "AppProject"));
            }
            if (!self->m_bStartupDocumentEdited && self->m_StartupDocumentEditor) {
                self->m_StartupDocumentEditor->SetText(NormalizeStartupDocumentName(text));
            }
            self->m_bUpdatingDerivedFields = false;
            self->SetErrorMessage("");
        }
    });

    namespaceEditor->OnTextChanged.AddLambda([weakThis](ImEditableText&, const std::string&) {
        if (auto self = weakThis.lock()) {
            if (!self->m_bUpdatingDerivedFields) {
                self->m_bNamespaceEdited = true;
            }
            self->SetErrorMessage("");
        }
    });

    startupDocumentEditor->OnTextChanged.AddLambda([weakThis](ImEditableText&, const std::string&) {
        if (auto self = weakThis.lock()) {
            if (!self->m_bUpdatingDerivedFields) {
                self->m_bStartupDocumentEdited = true;
            }
            self->SetErrorMessage("");
        }
    });

    confirmButton->OnClicked.AddLambda([requestConfirm](ImButton&) {
        requestConfirm();
    });
    cancelButton->OnClicked.AddLambda([requestCancel](ImButton&) {
        requestCancel();
    });
    projectNameEditor->OnTextCommitted.AddLambda([requestConfirm](ImEditableText& sender, const std::string&) {
        if (sender.HasKeyboardFocus()) {
            requestConfirm();
        }
    });
    namespaceEditor->OnTextCommitted.AddLambda([requestConfirm](ImEditableText& sender, const std::string&) {
        if (sender.HasKeyboardFocus()) {
            requestConfirm();
        }
    });
    startupDocumentEditor->OnTextCommitted.AddLambda([requestConfirm](ImEditableText& sender, const std::string&) {
        if (sender.HasKeyboardFocus()) {
            requestConfirm();
        }
    });

    FPopupOptions popupOptions;
    popupOptions.Title = m_Options.PopupTitle;
    popupOptions.Position = m_Options.Position;
    popupOptions.Size = MaxSize(m_Options.Size, popupContentMinSize);
    popupOptions.RootWidget = root;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style = MakeEditorPopupWindowStyle();

    m_Window = app.GetWindowManager().CreatePopup(popupOptions);
    app.SetKeyboardFocus(projectNameEditor);
    projectNameEditor->SelectAll();
    return static_cast<bool>(m_Window);
}

void NewAppProjectDialog::Close(ImApplication& app)
{
    if (m_Window) {
        app.GetWindowManager().CloseWindow(m_Window);
    }
    Reset();
}

bool NewAppProjectDialog::IsOpen() const
{
    return m_Window && m_Window->IsOpen();
}

void NewAppProjectDialog::Reset()
{
    m_Window.reset();
    m_Root.reset();
    m_ProjectNameEditor.reset();
    m_NamespaceEditor.reset();
    m_StartupDocumentEditor.reset();
    m_TemplateComboBox.reset();
    m_ErrorText.reset();
    m_ConfirmButton.reset();
    m_CancelButton.reset();
    m_Options = FNewAppProjectDialogOptions();
    m_bNamespaceEdited = false;
    m_bStartupDocumentEdited = false;
    m_bUpdatingDerivedFields = false;
}

void NewAppProjectDialog::SetErrorMessage(const std::string& message)
{
    if (m_ErrorText) {
        m_ErrorText->SetText(message);
    }
}

FCreateAppProjectOptions NewAppProjectDialog::BuildCurrentOptions() const
{
    FCreateAppProjectOptions options;
    if (m_ProjectNameEditor) {
        options.ProjectName = m_ProjectNameEditor->GetText();
    }
    if (m_NamespaceEditor) {
        options.NamespaceName = m_NamespaceEditor->GetText();
    }
    if (m_StartupDocumentEditor) {
        options.StartupDocumentName = m_StartupDocumentEditor->GetText();
    }
    if (m_TemplateComboBox && m_TemplateComboBox->HasSelection()) {
        options.TemplateName = m_TemplateComboBox->GetSelectedText();
    } else if (!m_Options.TemplateOptions.empty()) {
        options.TemplateName = m_Options.TemplateOptions.front();
    }
    return options;
}

} // namespace ImWidgetV4Editor
