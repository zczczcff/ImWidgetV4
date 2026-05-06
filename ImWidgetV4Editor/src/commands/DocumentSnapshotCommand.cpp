#include "DocumentSnapshotCommand.h"

#include "../editor/EditorSession.h"

namespace ImWidgetV4Editor {

DocumentSnapshotCommand::DocumentSnapshotCommand(
    const std::shared_ptr<EditorSession>& session,
    std::string label,
    json beforeDocument,
    std::vector<int> beforeSelectionPath,
    bool bBeforeDirty,
    json afterDocument,
    std::vector<int> afterSelectionPath,
    bool bAfterDirty)
    : EditorCommand(std::move(label))
    , m_Session(session)
    , m_BeforeDocument(std::move(beforeDocument))
    , m_BeforeSelectionPath(std::move(beforeSelectionPath))
    , m_bBeforeDirty(bBeforeDirty)
    , m_AfterDocument(std::move(afterDocument))
    , m_AfterSelectionPath(std::move(afterSelectionPath))
    , m_bAfterDirty(bAfterDirty)
{
}

bool DocumentSnapshotCommand::Execute()
{
    return Apply(m_AfterDocument, m_AfterSelectionPath, m_bAfterDirty);
}

bool DocumentSnapshotCommand::Undo()
{
    return Apply(m_BeforeDocument, m_BeforeSelectionPath, m_bBeforeDirty);
}

bool DocumentSnapshotCommand::Apply(
    const json& documentJson,
    const std::vector<int>& selectionPath,
    bool bDirty)
{
    std::shared_ptr<EditorSession> session = m_Session.lock();
    return session && session->ApplyDocumentSnapshot(documentJson, selectionPath, bDirty);
}

} // namespace ImWidgetV4Editor
