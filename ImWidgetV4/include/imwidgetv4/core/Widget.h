#pragma once

#include <imwidgetv4/core/DragDrop.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Reply.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/input/Input.h>
#include <memory>
#include <string>
#include <vector>

#ifdef GetClassName
#undef GetClassName
#endif

namespace ImWidgetV4 {

class ImApplication;
class ImWindow;
class ImWindowManager;
class ImSlot;
class ImPaddingSlot;

class ImWidget : public ReflectableObject, public std::enable_shared_from_this<ImWidget> {
    DECLARE_OBJECT_WITH_PARENT(ImWidget, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::String, "Name", &ImWidget::m_Name, "Widget display name")
        .RegisterProperty(PropertyType::Bool, "Visible", &ImWidget::m_bVisible, "Whether the widget is visible")
        .RegisterProperty(PropertyType::Bool, "HitTestVisible", &ImWidget::m_bHitTestVisible, "Whether the widget participates in hit testing")
        .RegisterProperty(PropertyType::Bool, "SupportsKeyboardFocus", &ImWidget::m_bSupportsKeyboardFocus, "Whether the widget can receive keyboard focus");
    END_DECLARE_OBJECT()

public:
    using Ptr = std::shared_ptr<ImWidget>;

    ImWidget();
    virtual ~ImWidget() = default;

    void SetName(const std::string& name) { m_Name = name; }
    const std::string& GetName() const { return m_Name; }

    void SetVisible(bool bVisible) { m_bVisible = bVisible; }
    bool IsVisible() const { return m_bVisible; }

    void SetHitTestVisible(bool bVisible) { m_bHitTestVisible = bVisible; }
    bool IsHitTestVisible() const { return m_bHitTestVisible; }

    void SetSupportsKeyboardFocus(bool bSupports) { m_bSupportsKeyboardFocus = bSupports; }
    bool SupportsKeyboardFocus() const { return m_bSupportsKeyboardFocus; }
    bool HasKeyboardFocus() const { return m_bHasKeyboardFocus; }

    void SetToolTipText(const std::string& toolTipText);
    void SetToolTipText(const FText& toolTipText);
    std::string GetToolTipText() const;
    const FText& GetToolTip() const { return m_ToolTipText; }
    void SetToolTipWidget(const Ptr& toolTipWidget);
    const Ptr& GetToolTipWidget() const { return m_ToolTipWidget; }
    void ClearToolTip();
    bool HasToolTip() const;

    void Invalidate(EInvalidateReason reason);
    virtual void OnFocusChanged(bool bHasFocus);

    virtual void Paint(const FPaintContext& paintContext);
    virtual FVector2 GetMinSize() const;

    void SetGeometry(const FGeometry& geometry) { m_Geometry = geometry; }
    const FGeometry& GetGeometry() const { return m_Geometry; }

    virtual std::shared_ptr<FDragDropOperation> OnDragDetected(const FDragDetectEvent& event);
    virtual FReply OnPreviewDragEvent(const FDragDropEvent& event);
    virtual FReply OnDragEvent(const FDragDropEvent& event);

    virtual FReply OnPreviewInputEvent(const FInputEvent& event);
    virtual FReply OnInputEvent(const FInputEvent& event);
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath);

    virtual void AddChild(const Ptr& child);
    virtual void InsertChildAt(int index, const Ptr& child);
    virtual bool RemoveChild(const Ptr& child);
    virtual void ClearChildren();

    const std::vector<Ptr>& GetChildren() const { return m_Children; }
    std::shared_ptr<ImWidget> GetParent() const;
    ImApplication* GetApplication() const { return m_Application; }

private:
    friend class ImApplication;
    friend class ImWindow;
    friend class ImWindowManager;
    friend class ImSlot;
    friend class ImPaddingSlot;

    void NotifyFocusChanged(bool bHasFocus);
    void SetApplicationRecursive(ImApplication* application);

protected:
    std::string m_Name;
    bool m_bVisible = true;
    bool m_bHitTestVisible = true;
    bool m_bSupportsKeyboardFocus = false;
    bool m_bHasKeyboardFocus = false;
    FGeometry m_Geometry;
    std::vector<Ptr> m_Children;
    std::weak_ptr<ImWidget> m_Parent;
    ImApplication* m_Application = nullptr;
    FText m_ToolTipText;
    Ptr m_ToolTipWidget;
};

} // namespace ImWidgetV4
