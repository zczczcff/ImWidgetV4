#include "InputDialog.h"
#include "../inspector/PropertyEditorWidgets.h"

#include <imwidgetv4/core/WindowManager.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;
using namespace ImWidgetV4Editor::PropertyEditorWidgets;

namespace {

FButtonStyle MakeDialogButtonStyle(bool bPrimary)
{
    return bPrimary ? FButtonStyle::CreatePrimary() : FButtonStyle();
}

} // namespace

bool InputDialog::Open(ImApplication& app, const FInputDialogOptions& options)
{
    Close(app);
    m_Options = options;

    auto title = std::make_shared<ImTextBlock>();
    title->SetText(m_Options.HeadingText);
    title->SetWrapText(false);
    title->SetFontSize(16.0f);
    title->SetTextColor(FColor::FromBytes(238, 242, 247));

    auto editor = std::make_shared<ImEditableText>();
    ApplyInspectorEditableTextStyle(*editor, false);
    editor->SetText(m_Options.InitialText);

    auto confirmButton = std::make_shared<ImButton>();
    confirmButton->SetStyle(MakeDialogButtonStyle(true));
    confirmButton->SetText(m_Options.ConfirmText);

    auto cancelButton = std::make_shared<ImButton>();
    cancelButton->SetStyle(MakeDialogButtonStyle(false));
    cancelButton->SetText(m_Options.CancelText);

    auto buttonRow = std::make_shared<ImHorizontalBox>();
    buttonRow->SetSpacing(8.0f);
    buttonRow->AddChildFill(MakeInspectorFlexibleSpacer(), 1.0f, FMargin(0.0f));
    buttonRow->AddChild(confirmButton);
    buttonRow->AddChild(cancelButton);

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(10.0f);
    root->AddChild(title, FMargin(12.0f, 12.0f, 12.0f, 0.0f));
    root->AddChild(editor, FMargin(12.0f, 0.0f, 12.0f, 0.0f));
    root->AddChild(buttonRow, FMargin(12.0f, 0.0f, 12.0f, 12.0f));

    const std::weak_ptr<InputDialog> weakThis = weak_from_this();
    auto requestConfirm = [weakThis, &app]() {
        if (auto self = weakThis.lock()) {
            const std::string text = self->m_Editor ? self->m_Editor->GetText() : std::string();
            const std::function<bool(const std::string&)> onConfirm = self->m_Options.OnConfirm;
            if (onConfirm && !onConfirm(text)) {
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

    confirmButton->OnClicked.AddLambda([requestConfirm](ImButton&) {
        requestConfirm();
    });
    cancelButton->OnClicked.AddLambda([requestCancel](ImButton&) {
        requestCancel();
    });
    editor->OnTextCommitted.AddLambda([requestConfirm](ImEditableText&, const std::string&) {
        requestConfirm();
    });

    FPopupOptions popupOptions;
    popupOptions.Title = m_Options.PopupTitle;
    popupOptions.Position = m_Options.Position;
    popupOptions.Size = m_Options.Size;
    popupOptions.RootWidget = root;
    popupOptions.bCloseOnClickOutside = true;
    popupOptions.Style.CornerRadius = 6.0f;
    popupOptions.Style.BorderThickness = 1.0f;
    popupOptions.Style.bDrawShadow = false;

    m_Root = root;
    m_Editor = editor;
    m_ConfirmButton = confirmButton;
    m_CancelButton = cancelButton;
    m_Window = app.GetWindowManager().CreatePopup(popupOptions);

    app.SetKeyboardFocus(editor);
    if (m_Options.bSelectAllOnOpen && m_Editor) {
        m_Editor->SelectAll();
    }

    return static_cast<bool>(m_Window);
}

void InputDialog::Close(ImApplication& app)
{
    if (m_Window) {
        app.GetWindowManager().CloseWindow(m_Window);
    }
    Reset();
}

bool InputDialog::IsOpen() const
{
    return m_Window && m_Window->IsOpen();
}

void InputDialog::Reset()
{
    m_Window.reset();
    m_Root.reset();
    m_Editor.reset();
    m_ConfirmButton.reset();
    m_CancelButton.reset();
    m_Options = FInputDialogOptions();
}

} // namespace ImWidgetV4Editor
