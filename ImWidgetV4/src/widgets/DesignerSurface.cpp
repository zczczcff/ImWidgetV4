#include <imwidgetv4/widgets/DesignerSurface.h>

#include <imwidgetv4/core/DrawContext.h>

namespace ImWidgetV4 {

ImDesignerSurface::ImDesignerSurface()
    : ImUserWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

void ImDesignerSurface::SetContentRoot(const Ptr& rootWidget)
{
    SetRootWidget(rootWidget);
    if (m_SelectedWidget && !ContainsWidgetInContent(m_SelectedWidget)) {
        ClearSelection();
    }
}

std::shared_ptr<ImWidget> ImDesignerSurface::GetContentRoot() const
{
    return GetRootWidget();
}

void ImDesignerSurface::SetSelectedWidget(const std::shared_ptr<ImWidget>& widget)
{
    if (widget && !ContainsWidgetInContent(widget)) {
        return;
    }

    if (m_SelectedWidget == widget) {
        return;
    }

    m_SelectedWidget = widget;
    Invalidate(EInvalidateReason::Paint);
    OnSelectionChanged.Broadcast(*this, m_SelectedWidget);
}

void ImDesignerSurface::ClearSelection()
{
    SetSelectedWidget(nullptr);
}

void ImDesignerSurface::SetSelectionBorderColor(const FColor& color)
{
    if (m_SelectionBorderColor.ToImU32() == color.ToImU32()) {
        return;
    }

    m_SelectionBorderColor = color;
    Invalidate(EInvalidateReason::Paint);
}

void ImDesignerSurface::SetSelectionFillColor(const FColor& color)
{
    if (m_SelectionFillColor.ToImU32() == color.ToImU32()) {
        return;
    }

    m_SelectionFillColor = color;
    Invalidate(EInvalidateReason::Paint);
}

void ImDesignerSurface::SetSelectionBorderThickness(float thickness)
{
    if (m_SelectionBorderThickness == thickness) {
        return;
    }

    m_SelectionBorderThickness = thickness;
    Invalidate(EInvalidateReason::Paint);
}

void ImDesignerSurface::Paint(const FPaintContext& paintContext)
{
    ImUserWidget::Paint(paintContext);
    PaintSelectionOverlay(paintContext);
}

FReply ImDesignerSurface::OnInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::KeyDown &&
        event.Key == EKey::DeleteKey &&
        HasKeyboardFocus()) {
        OnDeleteRequested.Broadcast(*this);
        return FReply::Handled();
    }

    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Left &&
        m_Geometry.Contains(event.MousePosition)) {
        SetSelectedWidget(ResolveSelectableWidgetAt(event.MousePosition));
        return FReply::Handled().SetKeyboardFocus(shared_from_this());
    }

    return ImUserWidget::OnInputEvent(event);
}

FReply ImDesignerSurface::OnDragEvent(const FDragDropEvent& event)
{
    if (event.Type == EDragDropEventType::DragEnter ||
        event.Type == EDragDropEventType::DragOver) {
        return FReply::Handled();
    }

    if (event.Type == EDragDropEventType::Drop) {
        bool bHandled = false;
        OnDropReceived.Broadcast(*this, event.Operation, event.CurrentPosition, bHandled);
        return bHandled ? FReply::Handled() : FReply::Unhandled();
    }

    return FReply::Unhandled();
}

std::shared_ptr<ImWidget> ImDesignerSurface::ResolveSelectableWidgetAt(const FVector2& position) const
{
    std::shared_ptr<ImWidget> contentRoot = GetContentRoot();
    if (!contentRoot) {
        return nullptr;
    }

    std::vector<Ptr> hitPath;
    if (!contentRoot->BuildHitTestPath(position, hitPath) || hitPath.empty()) {
        return nullptr;
    }

    for (auto it = hitPath.rbegin(); it != hitPath.rend(); ++it) {
        if (*it && *it != contentRoot) {
            return *it;
        }
    }

    return contentRoot;
}

bool ImDesignerSurface::ContainsWidgetInContent(const std::shared_ptr<ImWidget>& widget) const
{
    return ContainsWidgetRecursive(GetContentRoot(), widget);
}

bool ImDesignerSurface::ContainsWidgetRecursive(
    const std::shared_ptr<ImWidget>& root,
    const std::shared_ptr<ImWidget>& target) const
{
    if (!root || !target) {
        return false;
    }

    if (root == target) {
        return true;
    }

    for (const auto& child : root->GetChildren()) {
        if (ContainsWidgetRecursive(child, target)) {
            return true;
        }
    }

    return false;
}

void ImDesignerSurface::PaintSelectionOverlay(const FPaintContext& paintContext) const
{
    if (!m_SelectedWidget) {
        return;
    }

    const FGeometry selectionGeometry = m_SelectedWidget->GetGeometry();
    if (!selectionGeometry.IsValid()) {
        return;
    }

    paintContext.DrawContext_.DrawRectFilled(
        selectionGeometry.GetMin(),
        selectionGeometry.GetMax(),
        m_SelectionFillColor,
        0.0f);
    paintContext.DrawContext_.DrawRect(
        selectionGeometry.GetMin(),
        selectionGeometry.GetMax(),
        m_SelectionBorderColor,
        0.0f,
        m_SelectionBorderThickness);
}

} // namespace ImWidgetV4
