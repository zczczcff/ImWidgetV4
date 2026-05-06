#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4Editor {

class EditorSession;

class EditorShellHost : public ImWidgetV4::ImUserWidget {
public:
    EditorShellHost();
    virtual ~EditorShellHost() = default;

    void SetSession(const std::shared_ptr<EditorSession>& session);

    virtual ImWidgetV4::FReply OnPreviewInputEvent(const ImWidgetV4::FInputEvent& event) override;

private:
    std::weak_ptr<EditorSession> m_Session;
};

} // namespace ImWidgetV4Editor
