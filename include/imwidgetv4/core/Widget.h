#pragma once

#include <imwidgetv4/core/Reply.h>
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/input/Input.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

class ImApplication;
class ImWindow;
class ImWindowManager;
class ImSlot;
class ImPaddingSlot;

class ImWidget : public std::enable_shared_from_this<ImWidget> {
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

    void Invalidate(EInvalidateReason reason);
    virtual void OnFocusChanged(bool bHasFocus);

    virtual void Paint(const FPaintContext& paintContext);
    virtual FVector2 GetMinSize() const;

    void SetGeometry(const FGeometry& geometry) { m_Geometry = geometry; }
    const FGeometry& GetGeometry() const { return m_Geometry; }

    virtual FReply OnPreviewInputEvent(const FInputEvent& event);
    virtual FReply OnInputEvent(const FInputEvent& event);
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath);

    virtual void AddChild(const Ptr& child);
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
};

} // namespace ImWidgetV4
