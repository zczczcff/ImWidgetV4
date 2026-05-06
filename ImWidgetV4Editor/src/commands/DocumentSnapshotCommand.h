#pragma once

#include "EditorCommand.h"
#include "../serialization/DocumentFormat.h"

#include <memory>
#include <vector>

namespace ImWidgetV4Editor {

class EditorSession;

class DocumentSnapshotCommand : public EditorCommand {
public:
    DocumentSnapshotCommand(
        const std::shared_ptr<EditorSession>& session,
        std::string label,
        json beforeDocument,
        std::string beforeSelectionId,
        bool bBeforeDirty,
        json afterDocument,
        std::string afterSelectionId,
        bool bAfterDirty);

    virtual bool Execute() override;
    virtual bool Undo() override;

private:
    bool Apply(const json& documentJson, const std::string& selectionId, bool bDirty);

    std::weak_ptr<EditorSession> m_Session;
    json m_BeforeDocument;
    std::string m_BeforeSelectionId;
    bool m_bBeforeDirty = false;
    json m_AfterDocument;
    std::string m_AfterSelectionId;
    bool m_bAfterDirty = false;
};

} // namespace ImWidgetV4Editor
