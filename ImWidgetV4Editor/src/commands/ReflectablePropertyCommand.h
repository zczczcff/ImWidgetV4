#pragma once

#include "EditorCommand.h"
#include "../serialization/DocumentFormat.h"

#include <memory>

namespace ImWidgetV4 {
class ImWidget;
class ReflectableObject;
}

namespace ImWidgetV4Editor {

class EditorSession;

class ReflectablePropertyCommand : public EditorCommand {
public:
    ReflectablePropertyCommand(
        const std::shared_ptr<EditorSession>& session,
        const std::shared_ptr<ImWidgetV4::ReflectableObject>& owner,
        std::string label,
        json beforeJson,
        json afterJson,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bBeforeDirty,
        bool bAfterDirty);

    virtual bool Execute() override;
    virtual bool Undo() override;

    std::weak_ptr<EditorSession> m_Session;
    std::weak_ptr<ImWidgetV4::ReflectableObject> m_Owner;
    std::weak_ptr<ImWidgetV4::ImWidget> m_PreferredSelection;
    json m_BeforeJson;
    json m_AfterJson;
    bool m_bBeforeDirty = false;
    bool m_bAfterDirty = true;
};

} // namespace ImWidgetV4Editor
