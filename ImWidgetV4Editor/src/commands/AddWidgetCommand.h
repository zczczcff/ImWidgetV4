#pragma once

#include "EditorCommand.h"

#include <imwidgetv4/core/Types.h>
#include <memory>

namespace ImWidgetV4 {
enum class ETextOutlineDropZone : std::uint8_t;
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorSession;

class AddWidgetCommand : public EditorCommand {
public:
    enum class EInsertionMode {
        DesignerDrop,
        TreeTarget,
        DuplicateInParent
    };

    AddWidgetCommand(
        const std::shared_ptr<EditorSession>& session,
        std::string label,
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const std::shared_ptr<ImWidgetV4::ImWidget>& insertionTarget,
        ImWidgetV4::FVector2 dropPosition,
        ImWidgetV4::ETextOutlineDropZone treeZone,
        EInsertionMode insertionMode,
        const std::shared_ptr<ImWidgetV4::ImWidget>& preferredSelection,
        bool bBeforeDirty,
        bool bAfterDirty,
        std::string duplicateTabTitleSuffix = std::string());

    virtual bool Execute() override;
    virtual bool Undo() override;

private:
    std::weak_ptr<EditorSession> m_Session;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Widget;
    std::weak_ptr<ImWidgetV4::ImWidget> m_InsertionTarget;
    std::weak_ptr<ImWidgetV4::ImWidget> m_PreferredSelection;
    ImWidgetV4::FVector2 m_DropPosition {0.0f, 0.0f};
    std::string m_DuplicateTabTitleSuffix;
    ImWidgetV4::ETextOutlineDropZone m_TreeZone;
    EInsertionMode m_InsertionMode = EInsertionMode::DesignerDrop;
    bool m_bBeforeDirty = false;
    bool m_bAfterDirty = true;
};

} // namespace ImWidgetV4Editor
