#include <imwidgetv4/widgets/DesignerSurface.h>

#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/CanvasPanel.h>

#include <algorithm>
#include <imgui.h>

namespace ImWidgetV4 {

namespace {

bool IsResizeHandle(EDesignerTransformHandle handle)
{
    return handle != EDesignerTransformHandle::None &&
        handle != EDesignerTransformHandle::Move;
}

} // namespace

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
    PaintDropPreviewOverlay(paintContext);
    PaintSelectionOverlay(paintContext);

    const EDesignerTransformHandle effectiveHandle =
        m_ActiveTransformHandle != EDesignerTransformHandle::None
            ? m_ActiveTransformHandle
            : m_HoveredTransformHandle;
    if (effectiveHandle != EDesignerTransformHandle::None) {
        UpdateCursorForTransformHandle(effectiveHandle);
    }
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
            UpdateCursorForTransformHandle(EDesignerTransformHandle::None);
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
    const EDesignerTransformHandle transformHandleAtCursor = HitTestTransformHandle(event.MousePosition);

    if (selectedBefore && hitWidget == selectedBefore) {
        if (transformHandleAtCursor != EDesignerTransformHandle::None &&
            BeginTransform(transformHandleAtCursor, event.MousePosition)) {
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
    if (event.Type == EInputEventType::MouseButtonDown &&
        event.MouseButton == EMouseButton::Right &&
        m_Geometry.Contains(event.MousePosition)) {
        std::shared_ptr<ImWidget> hitWidget = ResolveSelectableWidgetAt(event.MousePosition);
        SetSelectedWidget(hitWidget);
        OnContextMenuRequested.Broadcast(*this, hitWidget, event.MousePosition);
        return FReply::Handled().SetKeyboardFocus(shared_from_this());
    }

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
        std::shared_ptr<ImWidget> previewTarget;
        bool bAccepted = false;
        OnDropTest.Broadcast(*this, event.Operation, event.CurrentPosition, previewTarget, bAccepted);

        if (m_DropPreviewWidget != previewTarget || m_bDropPreviewAccepted != bAccepted) {
            m_DropPreviewWidget = previewTarget;
            m_bDropPreviewAccepted = bAccepted;
            Invalidate(EInvalidateReason::Paint);
        }

        return bAccepted ? FReply::Handled() : FReply::Unhandled();
    }

    if (event.Type == EDragDropEventType::DragLeave) {
        if (m_DropPreviewWidget || m_bDropPreviewAccepted) {
            m_DropPreviewWidget.reset();
            m_bDropPreviewAccepted = false;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();
    }

    if (event.Type == EDragDropEventType::Drop) {
        bool bHandled = false;
        OnDropReceived.Broadcast(*this, event.Operation, event.CurrentPosition, bHandled);
        if (m_DropPreviewWidget || m_bDropPreviewAccepted) {
            m_DropPreviewWidget.reset();
            m_bDropPreviewAccepted = false;
            Invalidate(EInvalidateReason::Paint);
        }
        return bHandled ? FReply::Handled() : FReply::Unhandled();
    }

    if (event.Type == EDragDropEventType::DragEnd) {
        if (m_DropPreviewWidget || m_bDropPreviewAccepted) {
            m_DropPreviewWidget.reset();
            m_bDropPreviewAccepted = false;
            Invalidate(EInvalidateReason::Paint);
        }
    }

    return FReply::Unhandled();
}

std::shared_ptr<ImWidget> ImDesignerSurface::ResolveSelectableWidgetAt(const FVector2& position) const
{
    if (m_SelectedWidget && HitTestTransformHandle(position) != EDesignerTransformHandle::None) {
        return m_SelectedWidget;
    }

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

FGeometry ImDesignerSurface::GetTransformHandleGeometry(EDesignerTransformHandle handle) const
{
    if (handle == EDesignerTransformHandle::None ||
        !m_SelectedWidget ||
        !m_SelectedWidget->GetGeometry().IsValid()) {
        return FGeometry();
    }

    std::shared_ptr<ImCanvasPanel> canvas;
    ImCanvasPanelSlot* slot = nullptr;
    if (!ResolveCanvasSelectionContext(canvas, slot) || !canvas || !slot) {
        return FGeometry();
    }

    const FGeometry selectionGeometry = m_SelectedWidget->GetGeometry();
    const float handleSize = m_TransformHandleSize;
    const float halfHandleSize = handleSize * 0.5f;
    const FVector2 min = selectionGeometry.GetMin();
    const FVector2 max = selectionGeometry.GetMax();
    const FVector2 center = selectionGeometry.GetCenter();

    FVector2 anchor = center;
    switch (handle) {
    case EDesignerTransformHandle::ResizeTopLeft:
        anchor = min;
        break;
    case EDesignerTransformHandle::ResizeTopCenter:
        anchor = FVector2(center.X, min.Y);
        break;
    case EDesignerTransformHandle::ResizeTopRight:
        anchor = FVector2(max.X, min.Y);
        break;
    case EDesignerTransformHandle::ResizeMiddleLeft:
        anchor = FVector2(min.X, center.Y);
        break;
    case EDesignerTransformHandle::Move:
        anchor = center;
        break;
    case EDesignerTransformHandle::ResizeMiddleRight:
        anchor = FVector2(max.X, center.Y);
        break;
    case EDesignerTransformHandle::ResizeBottomLeft:
        anchor = FVector2(min.X, max.Y);
        break;
    case EDesignerTransformHandle::ResizeBottomCenter:
        anchor = FVector2(center.X, max.Y);
        break;
    case EDesignerTransformHandle::ResizeBottomRight:
        anchor = max;
        break;
    default:
        break;
    }

    return FGeometry(
        anchor.X - halfHandleSize,
        anchor.Y - halfHandleSize,
        handleSize,
        handleSize);
}

void ImDesignerSurface::UpdateCursorForTransformHandle(EDesignerTransformHandle handle) const
{
    if (ImGui::GetCurrentContext() == nullptr) {
        return;
    }

    ImGuiMouseCursor cursorShape = ImGuiMouseCursor_Arrow;
    switch (handle) {
    case EDesignerTransformHandle::ResizeTopLeft:
    case EDesignerTransformHandle::ResizeBottomRight:
        cursorShape = ImGuiMouseCursor_ResizeNWSE;
        break;
    case EDesignerTransformHandle::ResizeTopRight:
    case EDesignerTransformHandle::ResizeBottomLeft:
        cursorShape = ImGuiMouseCursor_ResizeNESW;
        break;
    case EDesignerTransformHandle::ResizeTopCenter:
    case EDesignerTransformHandle::ResizeBottomCenter:
        cursorShape = ImGuiMouseCursor_ResizeNS;
        break;
    case EDesignerTransformHandle::ResizeMiddleLeft:
    case EDesignerTransformHandle::ResizeMiddleRight:
        cursorShape = ImGuiMouseCursor_ResizeEW;
        break;
    case EDesignerTransformHandle::Move:
        cursorShape = ImGuiMouseCursor_ResizeAll;
        break;
    case EDesignerTransformHandle::None:
    default:
        cursorShape = ImGuiMouseCursor_Arrow;
        break;
    }

    ImGui::SetMouseCursor(cursorShape);
}

void ImDesignerSurface::PaintDropPreviewOverlay(const FPaintContext& paintContext) const
{
    if (!m_bDropPreviewAccepted) {
        return;
    }

    FGeometry previewGeometry = m_DropPreviewWidget ? m_DropPreviewWidget->GetGeometry() : m_Geometry;
    if (!previewGeometry.IsValid()) {
        return;
    }

    paintContext.DrawContext_.DrawRectFilled(
        previewGeometry.GetMin(),
        previewGeometry.GetMax(),
        m_DropPreviewFillColor,
        0.0f);
    paintContext.DrawContext_.DrawRect(
        previewGeometry.GetMin(),
        previewGeometry.GetMax(),
        m_DropPreviewBorderColor,
        0.0f,
        2.0f);
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

    const EDesignerTransformHandle kHandles[] = {
        EDesignerTransformHandle::ResizeTopLeft,
        EDesignerTransformHandle::ResizeTopCenter,
        EDesignerTransformHandle::ResizeTopRight,
        EDesignerTransformHandle::ResizeMiddleLeft,
        EDesignerTransformHandle::Move,
        EDesignerTransformHandle::ResizeMiddleRight,
        EDesignerTransformHandle::ResizeBottomLeft,
        EDesignerTransformHandle::ResizeBottomCenter,
        EDesignerTransformHandle::ResizeBottomRight
    };

    for (EDesignerTransformHandle handle : kHandles) {
        const FGeometry handleGeometry = GetTransformHandleGeometry(handle);
        if (!handleGeometry.IsValid()) {
            continue;
        }

        const FColor handleFillColor =
            m_ActiveTransformHandle == handle
                ? m_SelectionBorderColor
                : (m_HoveredTransformHandle == handle
                       ? m_SelectionBorderColor.Lerp(FColor::White, 0.18f)
                       : m_SelectionBorderColor.Lerp(FColor::Black, 0.12f));

        paintContext.DrawContext_.DrawRectFilled(
            handleGeometry.GetMin(),
            handleGeometry.GetMax(),
            handleFillColor,
            0.0f);
        paintContext.DrawContext_.DrawRect(
            handleGeometry.GetMin(),
            handleGeometry.GetMax(),
            FColor::White,
            0.0f,
            1.0f);
    }
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

    const EDesignerTransformHandle kHandles[] = {
        EDesignerTransformHandle::ResizeTopLeft,
        EDesignerTransformHandle::ResizeTopCenter,
        EDesignerTransformHandle::ResizeTopRight,
        EDesignerTransformHandle::ResizeMiddleLeft,
        EDesignerTransformHandle::Move,
        EDesignerTransformHandle::ResizeMiddleRight,
        EDesignerTransformHandle::ResizeBottomLeft,
        EDesignerTransformHandle::ResizeBottomCenter,
        EDesignerTransformHandle::ResizeBottomRight
    };

    for (EDesignerTransformHandle handle : kHandles) {
        if (GetTransformHandleGeometry(handle).Contains(position)) {
            return handle;
        }
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
    UpdateCursorForTransformHandle(m_HoveredTransformHandle);
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
    m_TransformStartEffectiveRelativeSize = slot->GetAutoSize()
        ? FVector2(
              std::max(0.0f, m_SelectedWidget->GetGeometry().Size.X / canvas->GetGeometry().Size.X),
              std::max(0.0f, m_SelectedWidget->GetGeometry().Size.Y / canvas->GetGeometry().Size.Y))
        : slot->GetRelativeSize();
    m_bTransformStartAutoSize = slot->GetAutoSize();
    m_bTransformChanged = false;
    UpdateCursorForTransformHandle(m_ActiveTransformHandle);
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

    const FVector2 minSize = m_SelectedWidget->GetMinSize();
    const FVector2 minRelativeSize(
        canvasGeometry.Size.X > 0.0f ? minSize.X / canvasGeometry.Size.X : 0.0f,
        canvasGeometry.Size.Y > 0.0f ? minSize.Y / canvasGeometry.Size.Y : 0.0f);

    const float startLeft = m_TransformStartRelativePosition.X;
    const float startTop = m_TransformStartRelativePosition.Y;
    const float startRight = startLeft + m_TransformStartEffectiveRelativeSize.X;
    const float startBottom = startTop + m_TransformStartEffectiveRelativeSize.Y;

    float newLeft = startLeft;
    float newTop = startTop;
    float newRight = startRight;
    float newBottom = startBottom;

    if (m_ActiveTransformHandle == EDesignerTransformHandle::Move) {
        newLeft = std::clamp(
            startLeft + relativeDelta.X,
            0.0f,
            std::max(0.0f, 1.0f - m_TransformStartEffectiveRelativeSize.X));
        newTop = std::clamp(
            startTop + relativeDelta.Y,
            0.0f,
            std::max(0.0f, 1.0f - m_TransformStartEffectiveRelativeSize.Y));
        newRight = newLeft + m_TransformStartEffectiveRelativeSize.X;
        newBottom = newTop + m_TransformStartEffectiveRelativeSize.Y;
    } else {
        if (m_ActiveTransformHandle == EDesignerTransformHandle::ResizeTopLeft ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeTopCenter ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeTopRight) {
            newTop = std::clamp(
                startTop + relativeDelta.Y,
                0.0f,
                startBottom - std::max(0.0f, minRelativeSize.Y));
        }

        if (m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomLeft ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomCenter ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomRight) {
            newBottom = std::clamp(
                startBottom + relativeDelta.Y,
                startTop + std::max(0.0f, minRelativeSize.Y),
                1.0f);
        }

        if (m_ActiveTransformHandle == EDesignerTransformHandle::ResizeTopLeft ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeMiddleLeft ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomLeft) {
            newLeft = std::clamp(
                startLeft + relativeDelta.X,
                0.0f,
                startRight - std::max(0.0f, minRelativeSize.X));
        }

        if (m_ActiveTransformHandle == EDesignerTransformHandle::ResizeTopRight ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeMiddleRight ||
            m_ActiveTransformHandle == EDesignerTransformHandle::ResizeBottomRight) {
            newRight = std::clamp(
                startRight + relativeDelta.X,
                startLeft + std::max(0.0f, minRelativeSize.X),
                1.0f);
        }
    }

    const FVector2 newPosition(newLeft, newTop);
    const FVector2 newRelativeSize(
        std::max(0.0f, newRight - newLeft),
        std::max(0.0f, newBottom - newTop));

    bool bChanged = false;
    if (m_ActiveTransformHandle == EDesignerTransformHandle::Move) {
        bChanged = newPosition != slot->GetRelativePosition();
        slot->SetRelativePosition(newPosition);
    } else if (IsResizeHandle(m_ActiveTransformHandle)) {
        bChanged =
            slot->GetAutoSize() ||
            newPosition != slot->GetRelativePosition() ||
            newRelativeSize != slot->GetRelativeSize();
        slot->SetAutoSize(false);
        slot->SetRelativePosition(newPosition);
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
    UpdateCursorForTransformHandle(m_HoveredTransformHandle);
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
    UpdateCursorForTransformHandle(m_HoveredTransformHandle);
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

} // namespace ImWidgetV4
