#include "EditorWidgetTreeHost.h"

#include "EditorWorkspaceController.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/TextOutlineView.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

EditorWidgetTreeHost::EditorWidgetTreeHost()
    : ImUserWidget()
{
    SetHitTestVisible(true);
}

void EditorWidgetTreeHost::SetWorkspaceController(const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    m_WorkspaceController = workspaceController;
}

void EditorWidgetTreeHost::SetWidgetTreeView(const std::shared_ptr<ImTextOutlineView>& widgetTreeView)
{
    if (m_WidgetTreeView == widgetTreeView) {
        return;
    }

    m_WidgetTreeView = widgetTreeView;
    SetRootWidget(widgetTreeView);
}

FReply EditorWidgetTreeHost::OnPreviewInputEvent(const FInputEvent& event)
{
    if (event.Type != EInputEventType::KeyDown ||
        !event.Modifiers.bCtrl ||
        event.Modifiers.bAlt ||
        event.Modifiers.bSuper) {
        return ImUserWidget::OnPreviewInputEvent(event);
    }

    const std::shared_ptr<EditorWorkspaceController> workspaceController = m_WorkspaceController.lock();
    if (!workspaceController) {
        return ImUserWidget::OnPreviewInputEvent(event);
    }

    const std::shared_ptr<ImWidget> focusedWidget = GetApplication() ? GetApplication()->GetKeyboardFocus() : nullptr;
    if (!IsWidgetInSubtree(focusedWidget)) {
        return ImUserWidget::OnPreviewInputEvent(event);
    }

    bool bHandled = false;
    if (event.Key == EKey::X && !event.Modifiers.bShift) {
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

    return bHandled ? FReply::Handled() : ImUserWidget::OnPreviewInputEvent(event);
}

bool EditorWidgetTreeHost::IsWidgetInSubtree(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget || !m_WidgetTreeView) {
        return false;
    }

    for (std::shared_ptr<ImWidget> current = widget; current; current = current->GetParent()) {
        if (current == m_WidgetTreeView) {
            return true;
        }
    }

    return false;
}

} // namespace ImWidgetV4Editor
