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
        std::vector<int> beforeSelectionPath,
        bool bBeforeDirty,
        json afterDocument,
        std::vector<int> afterSelectionPath,
        bool bAfterDirty);

    virtual bool Execute() override;
    virtual bool Undo() override;

private:
    bool Apply(const json& documentJson, const std::vector<int>& selectionPath, bool bDirty);

    std::weak_ptr<EditorSession> m_Session;
    json m_BeforeDocument;
    std::vector<int> m_BeforeSelectionPath;
    bool m_bBeforeDirty = false;
    json m_AfterDocument;
    std::vector<int> m_AfterSelectionPath;
    bool m_bAfterDirty = false;
};

} // namespace ImWidgetV4Editor
