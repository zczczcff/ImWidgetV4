#include "AddWidgetCommand.h"

#include "../editor/DocumentEditService.h"
#include "../editor/EditorSession.h"
#include <imwidgetv4/widgets/TextOutlineView.h>

namespace ImWidgetV4Editor {

AddWidgetCommand::AddWidgetCommand(
    const std::shared_ptr<EditorSession>& session,
    std::string label,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    const std::shared_ptr<ImWidgetV4::ImWidget>& insertionTarget,
    ImWidgetV4::FVector2 dropPosition,
    ImWidgetV4::ETextOutlineDropZone treeZone,
    EInsertionMode insertionMode,
    const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
    bool bBeforeDirty,
    bool bAfterDirty,
    std::string duplicateTabTitleSuffix)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_Widget(widget)
    , m_InsertionTarget(insertionTarget)
    , m_PreferredSelection(preferredSelection)
    , m_DropPosition(dropPosition)
    , m_DuplicateTabTitleSuffix(std::move(duplicateTabTitleSuffix))
    , m_TreeZone(treeZone)
    , m_InsertionMode(insertionMode)
    , m_bBeforeDirty(bBeforeDirty)
    , m_bAfterDirty(bAfterDirty)
{
}

bool AddWidgetCommand::Execute()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> insertionTarget = m_InsertionTarget.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    if (!session || !m_Widget) {
        return false;
    }

    if (m_InsertionMode == EInsertionMode::TreeTarget) {
        return session->ApplyWidgetInsertionAtTreeTarget(
            m_Widget,
            insertionTarget,
            m_TreeZone,
            preferredSelection ? preferredSelection : m_Widget,
            m_bAfterDirty);
    }

    if (m_InsertionMode == EInsertionMode::DuplicateInParent) {
        std::string duplicateError;
        FDocumentDuplicateOptions duplicateOptions;
        duplicateOptions.TabTitleSuffix = m_DuplicateTabTitleSuffix;
        if (!TryDuplicateWidgetInParent(
            insertionTarget,
            preferredSelection,
            m_Widget,
            duplicateError,
            duplicateOptions)) {
            return false;
        }
        return session->ApplyInsertedWidgetRefresh(
            m_Widget,
            preferredSelection ? preferredSelection : m_Widget,
            m_bAfterDirty);
    }

    return session->ApplyWidgetInsertion(
        m_Widget,
        insertionTarget,
        m_DropPosition,
        preferredSelection ? preferredSelection : m_Widget,
        m_bAfterDirty);
}

bool AddWidgetCommand::Undo()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    if (!session || !m_Widget) {
        return false;
    }

    return session->ApplyWidgetRemoval(
        m_Widget,
        preferredSelection,
        m_bBeforeDirty);
}

} // namespace ImWidgetV4Editor
