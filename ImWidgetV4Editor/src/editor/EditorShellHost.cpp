#include "EditorShellHost.h"

#include "EditorSession.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/EditableText.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

EditorShellHost::EditorShellHost()
    : ImUserWidget()
{
    SetHitTestVisible(true);
}

void EditorShellHost::SetSession(const std::shared_ptr<EditorSession>& session)
{
    m_Session = session;
}

FReply EditorShellHost::OnPreviewInputEvent(const FInputEvent& event)
{
    if (event.Type != EInputEventType::KeyDown ||
        !event.Modifiers.bCtrl ||
        event.Modifiers.bAlt ||
        event.Modifiers.bSuper) {
        return ImUserWidget::OnPreviewInputEvent(event);
    }

    ImApplication* application = GetApplication();
    if (application) {
        std::shared_ptr<ImWidget> focusedWidget = application->GetKeyboardFocus();
        if (std::dynamic_pointer_cast<ImEditableText>(focusedWidget)) {
            return ImUserWidget::OnPreviewInputEvent(event);
        }
    }

    std::shared_ptr<EditorSession> session = m_Session.lock();
    if (!session) {
        return ImUserWidget::OnPreviewInputEvent(event);
    }

    bool bHandled = false;
    if (event.Key == EKey::Z) {
        bHandled = event.Modifiers.bShift
            ? session->Redo()
            : session->Undo();
    } else if (event.Key == EKey::Y && !event.Modifiers.bShift) {
        bHandled = session->Redo();
    }

    return bHandled
        ? FReply::Handled()
        : ImUserWidget::OnPreviewInputEvent(event);
}

} // namespace ImWidgetV4Editor
