#pragma once

#include "EditorCommand.h"
#include "../serialization/DocumentFormat.h"

#include <memory>

namespace ImWidgetV4 {
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorSession;

class RemoveWidgetCommand : public EditorCommand {
public:
    RemoveWidgetCommand(
        const std::shared_ptr<EditorSession>& session,
        std::string label,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& reinsertionTarget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bBeforeDirty,
        bool bAfterDirty);

    virtual bool Execute() override;
    virtual bool Undo() override;

private:
    bool CaptureRemovalState(const std::shared_ptr<EditorSession>& session);
    bool RestoreRemovedWidget(const std::shared_ptr<EditorSession>& session);

    std::weak_ptr<EditorSession> m_Session;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Widget;
    std::weak_ptr<ImWidgetV4::ImWidget> m_ReinsertionTarget;
    std::weak_ptr<ImWidgetV4::ImWidget> m_PreferredSelection;
    json m_SlotJson;
    int m_ReinsertionIndex = -1;
    bool m_bBeforeDirty = false;
    bool m_bAfterDirty = true;
    bool m_bCapturedRemovalState = false;
    bool m_bRemovedRootWidget = false;
};

} // namespace ImWidgetV4Editor
