#pragma once

#include "EditorCommand.h"

#include <memory>

namespace ImWidgetV4 {
enum class ETextOutlineDropZone : std::uint8_t;
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorSession;

class MoveWidgetCommand : public EditorCommand {
public:
    MoveWidgetCommand(
        const std::shared_ptr<EditorSession>& session,
        std::string label,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& beforeParent,
        int beforeInsertIndex,
        const std::shared_ptr<ImWidgetV4::ImWidget>& afterParent,
        int afterInsertIndex,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bBeforeDirty,
        bool bAfterDirty);

    virtual bool Execute() override;
    virtual bool Undo() override;

private:
    std::weak_ptr<EditorSession> m_Session;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Widget;
    std::weak_ptr<ImWidgetV4::ImWidget> m_BeforeParent;
    std::weak_ptr<ImWidgetV4::ImWidget> m_AfterParent;
    std::weak_ptr<ImWidgetV4::ImWidget> m_PreferredSelection;
    int m_BeforeInsertIndex = -1;
    int m_AfterInsertIndex = -1;
    bool m_bBeforeDirty = false;
    bool m_bAfterDirty = true;
};

} // namespace ImWidgetV4Editor
