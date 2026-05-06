#include "DocumentSnapshotCommand.h"

#include "../editor/EditorSession.h"

namespace ImWidgetV4Editor {

DocumentSnapshotCommand::DocumentSnapshotCommand(
    const std::shared_ptr<EditorSession>& session,
    std::string label,
    json beforeDocument,
    std::string beforeSelectionId,
    bool bBeforeDirty,
    json afterDocument,
    std::string afterSelectionId,
    bool bAfterDirty)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_BeforeDocument(std::move(beforeDocument))
    , m_BeforeSelectionId(std::move(beforeSelectionId))
    , m_bBeforeDirty(bBeforeDirty)
    , m_AfterDocument(std::move(afterDocument))
    , m_AfterSelectionId(std::move(afterSelectionId))
    , m_bAfterDirty(bAfterDirty)
{
}

bool DocumentSnapshotCommand::Execute()
{
    return Apply(m_AfterDocument, m_AfterSelectionId, m_bAfterDirty);
}

bool DocumentSnapshotCommand::Undo()
{
    return Apply(m_BeforeDocument, m_BeforeSelectionId, m_bBeforeDirty);
}

bool DocumentSnapshotCommand::Apply(
    const json& documentJson,
    const std::string& selectionId,
    bool bDirty)
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    return session && session->ApplyDocumentSnapshot(documentJson, selectionId, bDirty);
}

} // namespace ImWidgetV4Editor
