#include "ReflectablePropertyCommand.h"

#include "../editor/EditorSession.h"

namespace ImWidgetV4Editor {

ReflectablePropertyCommand::ReflectablePropertyCommand(
    const std::shared_ptr<EditorSession>& session,
    const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
    std::string label,
    json beforeJson,
    json afterJson,
    const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
    bool bBeforeDirty,
    bool bAfterDirty)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_Owner(owner)
    , m_PreferredSelection(preferredSelection)
    , m_BeforeJson(std::move(beforeJson))
    , m_AfterJson(std::move(afterJson))
    , m_bBeforeDirty(bBeforeDirty)
    , m_bAfterDirty(bAfterDirty)
{
}

bool ReflectablePropertyCommand::Execute()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ReflectableObject> owner = m_Owner.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    return session && owner &&
        session->ApplyReflectablePropertyChange(owner, m_AfterJson, preferredSelection, m_bAfterDirty);
}

bool ReflectablePropertyCommand::Undo()
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    std::shared_ptr<ImWidgetV4::ReflectableObject> owner = m_Owner.lock();
    std::shared_ptr<ImWidgetV4::ImWidget> preferredSelection = m_PreferredSelection.lock();
    return session && owner &&
        session->ApplyReflectablePropertyChange(owner, m_BeforeJson, preferredSelection, m_bBeforeDirty);
}

} // namespace ImWidgetV4Editor
