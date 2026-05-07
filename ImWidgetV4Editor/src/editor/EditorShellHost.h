#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4Editor {

class EditorSession;
class EditorWorkspaceController;

class EditorShellHost : public ImWidgetV4::ImUserWidget {
public:
    EditorShellHost();
    virtual ~EditorShellHost() = default;

    void SetSession(const std::shared_ptr<EditorSession>& session);
    void SetWorkspaceController(const std::shared_ptr<EditorWorkspaceController>& workspaceController);

    virtual ImWidgetV4::FReply OnPreviewInputEvent(const ImWidgetV4::FInputEvent& event) override;

private:
    std::weak_ptr<EditorSession> m_Session;
    std::weak_ptr<EditorWorkspaceController> m_WorkspaceController;
};

} // namespace ImWidgetV4Editor
