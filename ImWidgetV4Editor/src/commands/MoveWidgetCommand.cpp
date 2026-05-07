#include "MoveWidgetCommand.h"

#include "../editor/EditorSession.h"

namespace ImWidgetV4Editor {

MoveWidgetCommand::MoveWidgetCommand(
    const std::shared_ptr<EditorSession>& session,
    std::string label,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    const std::shared_ptr<ImWidgetV4::ImWidget>& beforeParent,
    int beforeInsertIndex,
    const std::shared_ptr<ImWidgetV4::ImWidget>& afterParent,
    int afterInsertIndex,
    const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
    bool bBeforeDirty,
    bool bAfterDirty)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_Widget(widget)
    , m_BeforeParent(beforeParent)
    , m_AfterParent(afterParent)
    , m_PreferredSelection(preferredSelection)
    , m_BeforeInsertIndex(beforeInsertIndex)
    , m_AfterInsertIndex(afterInsertIndex)
    , m_bBeforeDirty(bBeforeDirty)
    , m_bAfterDirty(bAfterDirty)
{
}

bool MoveWidgetCommand::Execute()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> afterParent = m_AfterParent.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    if (!session || !m_Widget || !afterParent || m_AfterInsertIndex < 0) {
        return false;
    }

    return session->ApplyWidgetMoveAtParentIndex(
        m_Widget,
        afterParent,
        m_AfterInsertIndex,
        preferredSelection ? preferredSelection : m_Widget,
        m_bAfterDirty);
}

bool MoveWidgetCommand::Undo()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> beforeParent = m_BeforeParent.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    if (!session || !m_Widget || !beforeParent || m_BeforeInsertIndex < 0) {
        return false;
    }

    return session->ApplyWidgetMoveAtParentIndex(
        m_Widget,
        beforeParent,
        m_BeforeInsertIndex,
        preferredSelection ? preferredSelection : m_Widget,
        m_bBeforeDirty);
}

} // namespace ImWidgetV4Editor
