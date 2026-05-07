#include "EditorShellHost.h"

#include "EditorSession.h"
#include "EditorWorkspaceController.h"

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

void EditorShellHost::SetWorkspaceController(const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    m_WorkspaceController = workspaceController;
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

    std::shared_ptr<EditorWorkspaceController> workspaceController = m_WorkspaceController.lock();
    if (!workspaceController) {
        return ImUserWidget::OnPreviewInputEvent(event);
    }

    bool bHandled = false;
    if (event.Key == EKey::N && !event.Modifiers.bShift) {
        bHandled = workspaceController->NewDocument();
    } else if (event.Key == EKey::O && !event.Modifiers.bShift) {
        bHandled = application && workspaceController->OpenDocument(*application);
    } else if (event.Key == EKey::S) {
        bHandled = application && (event.Modifiers.bShift
            ? workspaceController->SaveDocumentAs(*application)
            : workspaceController->SaveDocument(*application));
    } else if (event.Key == EKey::W && !event.Modifiers.bShift) {
        bHandled = application && workspaceController->CloseActiveDocument(*application);
    } else if (event.Key == EKey::Tab) {
        bHandled = workspaceController->ActivateAdjacentDocument(event.Modifiers.bShift ? -1 : 1);
    } else if (event.Key == EKey::X && !event.Modifiers.bShift) {
        bHandled = workspaceController->CutSelectedWidget();
    } else if (event.Key == EKey::C && !event.Modifiers.bShift) {
        bHandled = workspaceController->CopySelectedWidget();
    } else if (event.Key == EKey::V && !event.Modifiers.bShift) {
        bHandled = workspaceController->PasteCopiedWidget();
    } else if (event.Key == EKey::Z) {
        bHandled = event.Modifiers.bShift
            ? workspaceController->Redo()
            : workspaceController->Undo();
    } else if (event.Key == EKey::Y && !event.Modifiers.bShift) {
        bHandled = workspaceController->Redo();
    } else if (event.Key == EKey::D && !event.Modifiers.bShift) {
        bHandled = workspaceController->DuplicateSelectedWidget();
    }

    return bHandled
        ? FReply::Handled()
        : ImUserWidget::OnPreviewInputEvent(event);
}

} // namespace ImWidgetV4Editor
