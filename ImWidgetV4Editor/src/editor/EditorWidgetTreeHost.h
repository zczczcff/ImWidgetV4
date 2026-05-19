#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
class ImTextOutlineView;
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorWorkspaceController;

class EditorWidgetTreeHost : public ImWidgetV4::ImUserWidget {
public:
    EditorWidgetTreeHost();
    virtual ~EditorWidgetTreeHost() = default;

    void SetWorkspaceController(const std::shared_ptr<EditorWorkspaceController>& workspaceController);
    void SetWidgetTreeView(const std::shared_ptr<ImWidgetV4::ImTextOutlineView>& widgetTreeView);
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> GetWidgetTreeView() const { return m_WidgetTreeView; }

    virtual ImWidgetV4::FReply OnPreviewInputEvent(const ImWidgetV4::FInputEvent& event) override;

private:
    bool IsWidgetInSubtree(const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;

    std::weak_ptr<EditorWorkspaceController> m_WorkspaceController;
    std::shared_ptr<ImWidgetV4::ImTextOutlineView> m_WidgetTreeView;
};

} // namespace ImWidgetV4Editor
