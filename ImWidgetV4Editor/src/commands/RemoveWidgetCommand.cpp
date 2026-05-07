#include "RemoveWidgetCommand.h"

#include "../editor/EditorSession.h"

namespace ImWidgetV4Editor {

RemoveWidgetCommand::RemoveWidgetCommand(
    const std::shared_ptr<EditorSession>& session,
    std::string label,
    const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
    const std::shared_ptr<ImWidgetV4::ImWidget>& reinsertionTarget,
    const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
    bool bBeforeDirty,
    bool bAfterDirty)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_Widget(widget)
    , m_ReinsertionTarget(reinsertionTarget)
    , m_PreferredSelection(preferredSelection)
    , m_bBeforeDirty(bBeforeDirty)
    , m_bAfterDirty(bAfterDirty)
{
}

bool RemoveWidgetCommand::Execute()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    if (!session || !m_Widget) {
        return false;
    }

    return session->ApplyWidgetRemoval(
        m_Widget,
        preferredSelection,
        m_bAfterDirty);
}

bool RemoveWidgetCommand::Undo()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> reinsertionTarget = m_ReinsertionTarget.lock();
    if (!session || !m_Widget || !reinsertionTarget) {
        return false;
    }

    return session->ApplyWidgetInsertion(
        m_Widget,
        reinsertionTarget,
        reinsertionTarget->GetGeometry().Position,
        m_Widget,
        m_bBeforeDirty);
}

} // namespace ImWidgetV4Editor
