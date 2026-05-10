#include <imwidgetv4/core/Widget.h>
#include <algorithm>

namespace ImWidgetV4 {

ImWidget::ImWidget()
    : m_Name("")
    , m_bVisible(true)
    , m_bHitTestVisible(true)
    , m_bSupportsKeyboardFocus(false)
    , m_bHasKeyboardFocus(false)
    , m_Geometry()
{
}

void ImWidget::Paint(const FPaintContext& paintContext)
{
}

FVector2 ImWidget::GetMinSize() const
{
    return FVector2(0.0f, 0.0f);
}

std::shared_ptr<FDragDropOperation> ImWidget::OnDragDetected(const FDragDetectEvent& event)
{
    return nullptr;
}

FReply ImWidget::OnPreviewDragEvent(const FDragDropEvent& event)
{
    return FReply::Unhandled();
}

FReply ImWidget::OnDragEvent(const FDragDropEvent& event)
{
    return FReply::Unhandled();
}

FReply ImWidget::OnPreviewInputEvent(const FInputEvent& event)
{
    return FReply::Unhandled();
}

FReply ImWidget::OnInputEvent(const FInputEvent& event)
{
    return FReply::Unhandled();
}

bool ImWidget::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    outPath.push_back(shared_from_this());

    for (auto it = m_Children.rbegin(); it != m_Children.rend(); ++it) {
        if ((*it)->BuildHitTestPath(position, outPath)) {
            return true;
        }
    }

    return true;
}

void ImWidget::AddChild(const Ptr& child)
{
    if (!child) {
        return;
    }

    child->m_Parent = weak_from_this();
    child->SetApplicationRecursive(m_Application);
    m_Children.push_back(child);
}

void ImWidget::InsertChildAt(int index, const Ptr& child)
{
    if (!child) {
        return;
    }

    const int clampedIndex = std::clamp(index, 0, static_cast<int>(m_Children.size()));
    child->m_Parent = weak_from_this();
    child->SetApplicationRecursive(m_Application);
    m_Children.insert(m_Children.begin() + clampedIndex, child);
}

bool ImWidget::RemoveChild(const Ptr& child)
{
    if (!child) {
        return false;
    }

    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it == m_Children.end()) {
        return false;
    }

    if (*it) {
        (*it)->m_Parent.reset();
        (*it)->SetApplicationRecursive(nullptr);
    }

    m_Children.erase(it);
    return true;
}

void ImWidget::ClearChildren()
{
    if (m_Children.empty()) {
        return;
    }

    for (const Ptr& child : m_Children) {
        if (child) {
            child->m_Parent.reset();
            child->SetApplicationRecursive(nullptr);
        }
    }

    m_Children.clear();
}

std::shared_ptr<ImWidget> ImWidget::GetParent() const
{
    return m_Parent.lock();
}

void ImWidget::Invalidate(EInvalidateReason reason)
{
}

void ImWidget::SetToolTipText(const std::string& toolTipText)
{
    SetToolTipText(FText::FromString(toolTipText));
}

void ImWidget::SetToolTipText(const FText& toolTipText)
{
    if (m_ToolTipText == toolTipText) {
        return;
    }

    m_ToolTipText = toolTipText;
}

std::string ImWidget::GetToolTipText() const
{
    return m_ToolTipText.Resolve();
}

void ImWidget::SetToolTipWidget(const Ptr& toolTipWidget)
{
    if (m_ToolTipWidget == toolTipWidget) {
        return;
    }

    m_ToolTipWidget = toolTipWidget;
}

void ImWidget::ClearToolTip()
{
    m_ToolTipText = FText();
    m_ToolTipWidget.reset();
}

bool ImWidget::HasToolTip() const
{
    return !GetToolTipText().empty() || m_ToolTipWidget != nullptr;
}

void ImWidget::NotifyFocusChanged(bool bHasFocus)
{
    if (m_bHasKeyboardFocus == bHasFocus) {
        return;
    }

    m_bHasKeyboardFocus = bHasFocus;
    Invalidate(EInvalidateReason::Focus | EInvalidateReason::Paint);
    OnFocusChanged(bHasFocus);
}

void ImWidget::OnFocusChanged(bool bHasFocus)
{
}

void ImWidget::SetApplicationRecursive(ImApplication* application)
{
    m_Application = application;
    for (const Ptr& child : m_Children) {
        if (child) {
            child->SetApplicationRecursive(application);
        }
    }
}

} // namespace ImWidgetV4
