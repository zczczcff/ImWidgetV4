#include "EditorDesignerSurfaceHost.h"

#include "EditorWorkspaceController.h"

#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/DesignerSurface.h>
#include <imwidgetv4/widgets/EditableText.h>
#include <imwidgetv4/widgets/TextList.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

EditorDesignerSurfaceHost::EditorDesignerSurfaceHost()
    : ImUserWidget()
{
    SetHitTestVisible(true);
}

void EditorDesignerSurfaceHost::SetWorkspaceController(const std::shared_ptr<EditorWorkspaceController>& workspaceController)
{
    m_WorkspaceController = workspaceController;
}

void EditorDesignerSurfaceHost::SetDesignerSurface(const std::shared_ptr<ImDesignerSurface>& designerSurface)
{
    if (m_DesignerSurface == designerSurface) {
        return;
    }

    m_DesignerSurface = designerSurface;
    SetRootWidget(designerSurface);
}

FReply EditorDesignerSurfaceHost::OnPreviewInputEvent(const FInputEvent& event)
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
    if (!IsWidgetInSubtree(focusedWidget) || IsTextInputWidget(focusedWidget)) {
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

bool EditorDesignerSurfaceHost::IsWidgetInSubtree(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget || !m_DesignerSurface) {
        return false;
    }

    for (std::shared_ptr<ImWidget> current = widget; current; current = current->GetParent()) {
        if (current == m_DesignerSurface) {
            return true;
        }
    }

    return false;
}

bool EditorDesignerSurfaceHost::IsTextInputWidget(const std::shared_ptr<ImWidget>& widget) const
{
    return std::dynamic_pointer_cast<ImEditableText>(widget) != nullptr ||
        std::dynamic_pointer_cast<ImTextList>(widget) != nullptr;
}

} // namespace ImWidgetV4Editor
