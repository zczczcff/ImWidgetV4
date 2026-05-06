#include <imwidgetv4/widgets/DesignerSurface.h>

#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/CanvasPanel.h>

#include <algorithm>

namespace ImWidgetV4 {

ImDesignerSurface::ImDesignerSurface()
    : ImUserWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

void ImDesignerSurface::SetContentRoot(const Ptr& rootWidget)
{
    CancelTransform();
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

    CancelTransform();
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

FReply ImDesignerSurface::OnPreviewInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::MouseMove) {
        if (m_ActiveTransformHandle != EDesignerTransformHandle::None) {
            UpdateTransform(event.MousePosition);
            return FReply::Handled();
        }

        UpdateHoveredTransformHandle(event.MousePosition);
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseLeave) {
        if (m_ActiveTransformHandle == EDesignerTransformHandle::None) {
            m_HoveredTransformHandle = EDesignerTransformHandle::None;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();
    }

    if (event.Type == EInputEventType::MouseButtonUp &&
        event.MouseButton == EMouseButton::Left &&
        m_ActiveTransformHandle != EDesignerTransformHandle::None) {
        UpdateTransform(event.MousePosition);
        EndTransform();
        UpdateHoveredTransformHandle(event.MousePosition);
        return FReply::Handled().ReleaseMouseCapture();
    }

    if (event.Type != EInputEventType::MouseButtonDown ||
        event.MouseButton != EMouseButton::Left ||
        !m_Geometry.Contains(event.MousePosition)) {
        return FReply::Unhandled();
    }

    const std::shared_ptr<ImWidget> selectedBefore = m_SelectedWidget;
    const std::shared_ptr<ImWidget> hitWidget = ResolveSelectableWidgetAt(event.MousePosition);

    if (selectedBefore && hitWidget == selectedBefore) {
        EDesignerTransformHandle transformHandle = HitTestTransformHandle(event.MousePosition);
        if (transformHandle == EDesignerTransformHandle::None &&
            selectedBefore->GetGeometry().Contains(event.MousePosition)) {
            transformHandle = EDesignerTransformHandle::Move;
        }

        if (transformHandle != EDesignerTransformHandle::None &&
            BeginTransform(transformHandle, event.MousePosition)) {
            return FReply::Handled()
                .SetKeyboardFocus(shared_from_this())
                .CaptureMouse(shared_from_this(), EMouseButton::Left);
        }
    }

    SetSelectedWidget(hitWidget);
    UpdateHoveredTransformHandle(event.MousePosition);
    return FReply::Handled().SetKeyboardFocus(shared_from_this());
}

FReply ImDesignerSurface::OnInputEvent(const FInputEvent& event)
{
    if (event.Type == EInputEventType::KeyDown &&
        event.Key == EKey::DeleteKey &&
        HasKeyboardFocus()) {
        OnDeleteRequested.Broadcast(*this);
        return FReply::Handled();
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

    std::shared_ptr<ImCanvasPanel> canvas;
    ImCanvasPanelSlot* slot = nullptr;
    if (!ResolveCanvasSelectionContext(canvas, slot) || !slot) {
        return;
    }

    const float handleSize = std::max(6.0f, m_TransformHandleSize);
    const FVector2 handleMin(
        selectionGeometry.GetMax().X - handleSize,
        selectionGeometry.GetMax().Y - handleSize);
    const FVector2 handleMax = selectionGeometry.GetMax();

    const FColor handleFillColor =
        m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomRight
            ? m_SelectionBorderColor
            : (m_HoveredTransformHandle == EDesignerTransformHandle::ResizeBottomRight
                   ? m_SelectionBorderColor.Lerp(FColor::White, 0.18f)
                   : m_SelectionBorderColor.Lerp(FColor::Black, 0.12f));

    paintContext.DrawContext_.DrawRectFilled(
        handleMin,
        handleMax,
        handleFillColor,
        0.0f);
    paintContext.DrawContext_.DrawRect(
        handleMin,
        handleMax,
        FColor::White,
        0.0f,
        1.0f);
}

bool ImDesignerSurface::ResolveCanvasSelectionContext(
    std::shared_ptr<ImCanvasPanel>& outCanvas,
    ImCanvasPanelSlot*& outSlot) const
{
    outCanvas.reset();
    outSlot = nullptr;

    if (!m_SelectedWidget) {
        return false;
    }

    std::shared_ptr<ImWidget> parent = m_SelectedWidget->GetParent();
    if (!parent) {
        return false;
    }

    outCanvas = std::dynamic_pointer_cast<ImCanvasPanel>(parent);
    if (!outCanvas) {
        return false;
    }

    outSlot = dynamic_cast<ImCanvasPanelSlot*>(outCanvas->GetSlotForChild(m_SelectedWidget));
    return outSlot != nullptr;
}

EDesignerTransformHandle ImDesignerSurface::HitTestTransformHandle(const FVector2& position) const
{
    if (!m_SelectedWidget || !m_SelectedWidget->GetGeometry().IsValid()) {
        return EDesignerTransformHandle::None;
    }

    std::shared_ptr<ImCanvasPanel> canvas;
    ImCanvasPanelSlot* slot = nullptr;
    if (!ResolveCanvasSelectionContext(canvas, slot) || !canvas || !slot) {
        return EDesignerTransformHandle::None;
    }

    const FGeometry selectionGeometry = m_SelectedWidget->GetGeometry();
    const float handleSize = std::max(6.0f, m_TransformHandleSize);
    const FGeometry handleGeometry(
        selectionGeometry.GetMax().X - handleSize,
        selectionGeometry.GetMax().Y - handleSize,
        handleSize,
        handleSize);

    if (handleGeometry.Contains(position)) {
        return EDesignerTransformHandle::ResizeBottomRight;
    }

    return EDesignerTransformHandle::None;
}

void ImDesignerSurface::UpdateHoveredTransformHandle(const FVector2& position)
{
    const EDesignerTransformHandle newHandle = m_Geometry.Contains(position)
        ? HitTestTransformHandle(position)
        : EDesignerTransformHandle::None;
    if (newHandle == m_HoveredTransformHandle) {
        return;
    }

    m_HoveredTransformHandle = newHandle;
    Invalidate(EInvalidateReason::Paint);
}

bool ImDesignerSurface::BeginTransform(
    EDesignerTransformHandle handle,
    const FVector2& mousePosition)
{
    if (handle == EDesignerTransformHandle::None) {
        return false;
    }

    std::shared_ptr<ImCanvasPanel> canvas;
    ImCanvasPanelSlot* slot = nullptr;
    if (!ResolveCanvasSelectionContext(canvas, slot) || !canvas || !slot) {
        return false;
    }

    m_ActiveTransformHandle = handle;
    m_TransformStartMousePosition = mousePosition;
    m_TransformStartRelativePosition = slot->GetRelativePosition();
    m_TransformStartRelativeSize = slot->GetRelativeSize();
    m_bTransformStartAutoSize = slot->GetAutoSize();
    m_bTransformChanged = false;
    Invalidate(EInvalidateReason::Paint);
    OnTransformStarted.Broadcast(*this, m_SelectedWidget, handle);
    return true;
}

bool ImDesignerSurface::UpdateTransform(const FVector2& mousePosition)
{
    if (m_ActiveTransformHandle == EDesignerTransformHandle::None || !m_SelectedWidget) {
        return false;
    }

    std::shared_ptr<ImCanvasPanel> canvas;
    ImCanvasPanelSlot* slot = nullptr;
    if (!ResolveCanvasSelectionContext(canvas, slot) || !canvas || !slot) {
        return false;
    }

    const FGeometry canvasGeometry = canvas->GetGeometry();
    if (canvasGeometry.Size.X <= 0.0f || canvasGeometry.Size.Y <= 0.0f) {
        return false;
    }

    const FVector2 mouseDelta = mousePosition - m_TransformStartMousePosition;
    const FVector2 relativeDelta(
        mouseDelta.X / canvasGeometry.Size.X,
        mouseDelta.Y / canvasGeometry.Size.Y);

    bool bChanged = false;
    if (m_ActiveTransformHandle == EDesignerTransformHandle::Move) {
        const FVector2 currentSize = slot->GetAutoSize()
            ? FVector2(
                  std::max(0.0f, m_SelectedWidget->GetGeometry().Size.X / canvasGeometry.Size.X),
                  std::max(0.0f, m_SelectedWidget->GetGeometry().Size.Y / canvasGeometry.Size.Y))
            : m_TransformStartRelativeSize;
        const FVector2 newPosition(
            std::clamp(m_TransformStartRelativePosition.X + relativeDelta.X, 0.0f, std::max(0.0f, 1.0f - currentSize.X)),
            std::clamp(m_TransformStartRelativePosition.Y + relativeDelta.Y, 0.0f, std::max(0.0f, 1.0f - currentSize.Y)));
        bChanged = newPosition != slot->GetRelativePosition();
        slot->SetRelativePosition(newPosition);
    } else if (m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomRight) {
        const FVector2 minSize = m_SelectedWidget->GetMinSize();
        const FVector2 minRelativeSize(
            canvasGeometry.Size.X > 0.0f ? minSize.X / canvasGeometry.Size.X : 0.0f,
            canvasGeometry.Size.Y > 0.0f ? minSize.Y / canvasGeometry.Size.Y : 0.0f);
        const FVector2 maxRelativeSize(
            std::max(0.0f, 1.0f - m_TransformStartRelativePosition.X),
            std::max(0.0f, 1.0f - m_TransformStartRelativePosition.Y));
        const FVector2 newRelativeSize(
            std::clamp(m_TransformStartRelativeSize.X + relativeDelta.X, std::max(0.0f, minRelativeSize.X), maxRelativeSize.X),
            std::clamp(m_TransformStartRelativeSize.Y + relativeDelta.Y, std::max(0.0f, minRelativeSize.Y), maxRelativeSize.Y));
        bChanged =
            slot->GetAutoSize() ||
            newRelativeSize != slot->GetRelativeSize();
        slot->SetAutoSize(false);
        slot->SetRelativeSize(newRelativeSize);
    }

    m_bTransformChanged = m_bTransformChanged || bChanged;
    canvas->Relayout();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return bChanged;
}

void ImDesignerSurface::EndTransform()
{
    if (m_ActiveTransformHandle == EDesignerTransformHandle::None) {
        return;
    }

    const EDesignerTransformHandle completedHandle = m_ActiveTransformHandle;
    m_ActiveTransformHandle = EDesignerTransformHandle::None;
    Invalidate(EInvalidateReason::Paint);
    OnTransformFinished.Broadcast(*this, m_SelectedWidget, completedHandle, m_bTransformChanged);
    m_bTransformChanged = false;
}

void ImDesignerSurface::CancelTransform()
{
    if (m_ActiveTransformHandle == EDesignerTransformHandle::None) {
        m_bTransformChanged = false;
        return;
    }

    std::shared_ptr<ImCanvasPanel> canvas;
    ImCanvasPanelSlot* slot = nullptr;
    if (ResolveCanvasSelectionContext(canvas, slot) && canvas && slot) {
        slot->SetRelativePosition(m_TransformStartRelativePosition);
        slot->SetRelativeSize(m_TransformStartRelativeSize);
        slot->SetAutoSize(m_bTransformStartAutoSize);
        canvas->Relayout();
    }

    m_ActiveTransformHandle = EDesignerTransformHandle::None;
    m_bTransformChanged = false;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

} // namespace ImWidgetV4
