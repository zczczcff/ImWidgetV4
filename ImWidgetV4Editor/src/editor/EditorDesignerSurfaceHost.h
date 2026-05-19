#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
class ImDesignerSurface;
class ImEditableText;
class ImTextList;
class ImWidget;
}

namespace ImWidgetV4Editor {

class EditorWorkspaceController;

class EditorDesignerSurfaceHost : public ImWidgetV4::ImUserWidget {
public:
    EditorDesignerSurfaceHost();
    virtual ~EditorDesignerSurfaceHost() = default;

    void SetWorkspaceController(const std::shared_ptr<EditorWorkspaceController>& workspaceController);
    void SetDesignerSurface(const std::shared_ptr<ImWidgetV4::ImDesignerSurface>& designerSurface);
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> GetDesignerSurface() const { return m_DesignerSurface; }

    virtual ImWidgetV4::FReply OnPreviewInputEvent(const ImWidgetV4::FInputEvent& event) override;

private:
    bool IsWidgetInSubtree(const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;
    bool IsTextInputWidget(const std::shared_ptr<ImWidgetV4::ImWidget>& widget) const;

    std::weak_ptr<EditorWorkspaceController> m_WorkspaceController;
    std::shared_ptr<ImWidgetV4::ImDesignerSurface> m_DesignerSurface;
};

} // namespace ImWidgetV4Editor
