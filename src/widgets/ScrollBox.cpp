#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>
#include <cmath>

namespace ImWidgetV4 {

namespace {

FGeometry InsetGeometry(const FGeometry& geometry, float insetLeft, float insetTop, float insetRight, float insetBottom)
{
    const FVector2 min(
        geometry.Position.X + insetLeft,
        geometry.Position.Y + insetTop);
    const FVector2 max(
        geometry.Position.X + geometry.Size.X - insetRight,
        geometry.Position.Y + geometry.Size.Y - insetBottom);

    return FGeometry(
        min,
        FVector2(
            std::max(0.0f, max.X - min.X),
            std::max(0.0f, max.Y - min.Y)));
}

float ResolveVisibleOffset(float targetStart, float targetSize, float viewportSize, float currentOffset, bool bCenterIfLarger)
{
    if (viewportSize <= 0.0f) {
        return currentOffset;
    }

    if (targetSize > viewportSize) {
        if (bCenterIfLarger) {
            return targetStart - (viewportSize - targetSize) * 0.5f;
        }

        if (targetStart < currentOffset || targetStart + targetSize > currentOffset + viewportSize) {
            return targetStart;
        }

        return currentOffset;
    }

    if (targetStart < currentOffset) {
        return targetStart;
    }

    const float targetEnd = targetStart + targetSize;
    const float viewportEnd = currentOffset + viewportSize;
    if (targetEnd > viewportEnd) {
        return targetEnd - viewportSize;
    }

    return currentOffset;
}

} // namespace

ImScrollBox::ImScrollBox()
    : ImPanelWidget()
{
    SetHitTestVisible(true);
}

void ImScrollBox::SetContent(const std::shared_ptr<ImWidget>& child)
{
    if (m_Content == child) {
        return;
    }

    ImWidget::ClearChildren();
    m_Slots.clear();
    m_Content = child;
    m_ScrollOffset = FVector2(0.0f, 0.0f);
    m_MaxScrollOffset = FVector2(0.0f, 0.0f);
    m_HoveredScrollbar = EHoveredScrollbar::None;

    if (m_Content) {
        ImWidget::AddChild(m_Content);
    }

    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImScrollBox::AddChild(const Ptr& child)
{
    SetContent(child);
}

void ImScrollBox::ClearChildren()
{
    ImWidget::ClearChildren();
    m_Slots.clear();
    m_Content.reset();
    m_ScrollOffset = FVector2(0.0f, 0.0f);
    m_MaxScrollOffset = FVector2(0.0f, 0.0f);
    m_CachedContentSize = FVector2(0.0f, 0.0f);
    m_CachedViewportGeometry = FGeometry();
    m_HorizontalScrollbarGeometry = FGeometry();
    m_VerticalScrollbarGeometry = FGeometry();
    m_HorizontalThumbGeometry = FGeometry();
    m_VerticalThumbGeometry = FGeometry();
    m_bShowHorizontalScrollbar = false;
    m_bShowVerticalScrollbar = false;
    m_HoveredScrollbar = EHoveredScrollbar::None;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImScrollBox::SetScrollOffset(const FVector2& scrollOffset)
{
    Relayout();

    const FVector2 clamped(
        std::clamp(scrollOffset.X, 0.0f, m_MaxScrollOffset.X),
        std::clamp(scrollOffset.Y, 0.0f, m_MaxScrollOffset.Y));
    if (m_ScrollOffset == clamped) {
        return;
    }

    m_ScrollOffset = clamped;
    Relayout();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImScrollBox::ScrollBy(const FVector2& delta)
{
    SetScrollOffset(m_ScrollOffset + delta);
}

void ImScrollBox::ScrollToStart()
{
    SetScrollOffset(FVector2(0.0f, 0.0f));
}

void ImScrollBox::ScrollToEnd()
{
    Relayout();
    SetScrollOffset(m_MaxScrollOffset);
}

bool ImScrollBox::ScrollToWidget(const std::shared_ptr<ImWidget>& widget, bool bCenterIfLarger)
{
    Relayout();

    if (!widget || !m_Content || !IsDescendantOfContent(widget)) {
        return false;
    }

    if (m_CachedViewportGeometry.Size.X <= 0.0f || m_CachedViewportGeometry.Size.Y <= 0.0f) {
        return false;
    }

    const FGeometry targetGeometry = widget->GetGeometry();
    const FGeometry contentGeometry = m_Content->GetGeometry();
    const FVector2 targetLocalPosition(
        targetGeometry.Position.X - contentGeometry.Position.X,
        targetGeometry.Position.Y - contentGeometry.Position.Y);

    FVector2 nextOffset = m_ScrollOffset;
    nextOffset.X = ResolveVisibleOffset(
        targetLocalPosition.X,
        targetGeometry.Size.X,
        m_CachedViewportGeometry.Size.X,
        m_ScrollOffset.X,
        bCenterIfLarger);
    nextOffset.Y = ResolveVisibleOffset(
        targetLocalPosition.Y,
        targetGeometry.Size.Y,
        m_CachedViewportGeometry.Size.Y,
        m_ScrollOffset.Y,
        bCenterIfLarger);

    SetScrollOffset(nextOffset);
    return true;
}

void ImScrollBox::SetStyle(const FScrollBoxStyle& style)
{
    m_Style = style;
    Relayout();
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImScrollBox::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        m_Style.BackgroundColor,
        m_Style.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        m_Style.BorderColor,
        m_Style.CornerRadius,
        m_Style.BorderThickness);

    if (m_Content && m_CachedViewportGeometry.IsValid()) {
        paintContext.DrawContext_.PushClipRect(
            m_CachedViewportGeometry.GetMin(),
            m_CachedViewportGeometry.GetMax(),
            true);
        m_Content->Paint(paintContext);
        paintContext.DrawContext_.PopClipRect();
    }

    if (m_bShowHorizontalScrollbar && m_HorizontalScrollbarGeometry.IsValid()) {
        const bool bHorizontalActive = m_ActiveScrollbar == EActiveScrollbar::Horizontal;
        paintContext.DrawContext_.DrawRectFilled(
            m_HorizontalScrollbarGeometry.GetMin(),
            m_HorizontalScrollbarGeometry.GetMax(),
            m_Style.ScrollbarTrackColor,
            m_Style.ScrollbarThickness * 0.5f);
        paintContext.DrawContext_.DrawRectFilled(
            m_HorizontalThumbGeometry.GetMin(),
            m_HorizontalThumbGeometry.GetMax(),
            (bHorizontalActive || m_HoveredScrollbar == EHoveredScrollbar::Horizontal)
                ? m_Style.ScrollbarThumbHoveredColor
                : m_Style.ScrollbarThumbColor,
            m_Style.ScrollbarThickness * 0.5f);
    }

    if (m_bShowVerticalScrollbar && m_VerticalScrollbarGeometry.IsValid()) {
        const bool bVerticalActive = m_ActiveScrollbar == EActiveScrollbar::Vertical;
        paintContext.DrawContext_.DrawRectFilled(
            m_VerticalScrollbarGeometry.GetMin(),
            m_VerticalScrollbarGeometry.GetMax(),
            m_Style.ScrollbarTrackColor,
            m_Style.ScrollbarThickness * 0.5f);
        paintContext.DrawContext_.DrawRectFilled(
            m_VerticalThumbGeometry.GetMin(),
            m_VerticalThumbGeometry.GetMax(),
            (bVerticalActive || m_HoveredScrollbar == EHoveredScrollbar::Vertical)
                ? m_Style.ScrollbarThumbHoveredColor
                : m_Style.ScrollbarThumbColor,
            m_Style.ScrollbarThickness * 0.5f);
    }
}

FVector2 ImScrollBox::GetMinSize() const
{
    const float insetX = m_Style.Padding.Left + m_Style.Padding.Right + m_Style.BorderThickness * 2.0f;
    const float insetY = m_Style.Padding.Top + m_Style.Padding.Bottom + m_Style.BorderThickness * 2.0f;
    const FVector2 childMinSize = m_Content ? m_Content->GetMinSize() : FVector2(0.0f, 0.0f);
    return FVector2(childMinSize.X + insetX, childMinSize.Y + insetY);
}

FReply ImScrollBox::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    switch (event.Type) {
    case EInputEventType::MouseButtonDown:
        if (event.MouseButton == EMouseButton::Left) {
            if (m_bShowHorizontalScrollbar && m_HorizontalThumbGeometry.Contains(event.MousePosition)) {
                BeginScrollbarDrag(
                    EActiveScrollbar::Horizontal,
                    event.MousePosition.X - m_HorizontalThumbGeometry.Position.X);
                return FReply::Handled().CaptureMouse(shared_from_this(), EMouseButton::Left);
            }

            if (m_bShowVerticalScrollbar && m_VerticalThumbGeometry.Contains(event.MousePosition)) {
                BeginScrollbarDrag(
                    EActiveScrollbar::Vertical,
                    event.MousePosition.Y - m_VerticalThumbGeometry.Position.Y);
                return FReply::Handled().CaptureMouse(shared_from_this(), EMouseButton::Left);
            }
        }
        break;
    case EInputEventType::MouseButtonUp:
        if (event.MouseButton == EMouseButton::Left && m_ActiveScrollbar != EActiveScrollbar::None) {
            UpdateScrollbarDrag(event.MousePosition);
            EndScrollbarDrag();
            UpdateHoveredScrollbar(event.MousePosition);
            return FReply::Handled().ReleaseMouseCapture();
        }
        break;
    case EInputEventType::MouseEnter:
    case EInputEventType::MouseMove:
        if (m_ActiveScrollbar != EActiveScrollbar::None) {
            UpdateScrollbarDrag(event.MousePosition);
            return FReply::Handled();
        }
        UpdateHoveredScrollbar(event.MousePosition);
        return FReply::Unhandled();
    case EInputEventType::MouseLeave:
        if (m_ActiveScrollbar == EActiveScrollbar::None && m_HoveredScrollbar != EHoveredScrollbar::None) {
            m_HoveredScrollbar = EHoveredScrollbar::None;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();
    case EInputEventType::MouseWheel: {
        if (!m_Geometry.Contains(event.MousePosition)) {
            return FReply::Unhandled();
        }

        const bool bCanScrollHorizontally = m_MaxScrollOffset.X > 0.0f && event.ScrollDelta.X != 0.0f;
        const bool bCanScrollVertically = m_MaxScrollOffset.Y > 0.0f && event.ScrollDelta.Y != 0.0f;
        if (!bCanScrollHorizontally && !bCanScrollVertically) {
            return FReply::Unhandled();
        }

        const FVector2 oldOffset = m_ScrollOffset;
        FVector2 nextOffset = m_ScrollOffset;
        if (bCanScrollHorizontally) {
            nextOffset.X -= event.ScrollDelta.X * m_Style.WheelScrollStep;
        }
        if (bCanScrollVertically) {
            nextOffset.Y -= event.ScrollDelta.Y * m_Style.WheelScrollStep;
        }

        SetScrollOffset(nextOffset);
        return oldOffset != m_ScrollOffset ? FReply::Handled() : FReply::Unhandled();
    }
    default:
        break;
    }

    return FReply::Unhandled();
}

bool ImScrollBox::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    Relayout();
    outPath.push_back(shared_from_this());

    if (m_Content && m_CachedViewportGeometry.Contains(position)) {
        if (m_Content->BuildHitTestPath(position, outPath)) {
            return true;
        }
    }

    return true;
}

void ImScrollBox::Relayout()
{
    const float borderInset = std::max(0.0f, m_Style.BorderThickness);
    const float scrollbarThickness = std::max(0.0f, m_Style.ScrollbarThickness);
    const float scrollbarPadding = std::max(0.0f, m_Style.ScrollbarPadding);

    const FGeometry innerGeometry = InsetGeometry(
        m_Geometry,
        borderInset + m_Style.Padding.Left,
        borderInset + m_Style.Padding.Top,
        borderInset + m_Style.Padding.Right,
        borderInset + m_Style.Padding.Bottom);

    const FVector2 contentDesiredSize = m_Content ? m_Content->GetMinSize() : FVector2(0.0f, 0.0f);
    bool showHorizontal = false;
    bool showVertical = false;

    for (int pass = 0; pass < 3; ++pass) {
        FVector2 viewportSize = innerGeometry.Size;
        if (showVertical) {
            viewportSize.X = std::max(0.0f, viewportSize.X - scrollbarThickness - scrollbarPadding);
        }
        if (showHorizontal) {
            viewportSize.Y = std::max(0.0f, viewportSize.Y - scrollbarThickness - scrollbarPadding);
        }

        const bool nextHorizontal = contentDesiredSize.X > viewportSize.X + 0.5f;
        const bool nextVertical = contentDesiredSize.Y > viewportSize.Y + 0.5f;
        if (nextHorizontal == showHorizontal && nextVertical == showVertical) {
            break;
        }

        showHorizontal = nextHorizontal;
        showVertical = nextVertical;
    }

    m_bShowHorizontalScrollbar = showHorizontal;
    m_bShowVerticalScrollbar = showVertical;

    FVector2 viewportSize = innerGeometry.Size;
    if (m_bShowVerticalScrollbar) {
        viewportSize.X = std::max(0.0f, viewportSize.X - scrollbarThickness - scrollbarPadding);
    }
    if (m_bShowHorizontalScrollbar) {
        viewportSize.Y = std::max(0.0f, viewportSize.Y - scrollbarThickness - scrollbarPadding);
    }
    m_CachedViewportGeometry = FGeometry(innerGeometry.Position, viewportSize);

    m_CachedContentSize = FVector2(
        std::max(viewportSize.X, contentDesiredSize.X),
        std::max(viewportSize.Y, contentDesiredSize.Y));
    m_MaxScrollOffset = FVector2(
        std::max(0.0f, m_CachedContentSize.X - viewportSize.X),
        std::max(0.0f, m_CachedContentSize.Y - viewportSize.Y));
    ClampScrollOffset();

    if (m_Content) {
        m_Content->SetGeometry(FGeometry(
            FVector2(
                m_CachedViewportGeometry.Position.X - m_ScrollOffset.X,
                m_CachedViewportGeometry.Position.Y - m_ScrollOffset.Y),
            m_CachedContentSize));
    }

    m_HorizontalScrollbarGeometry = FGeometry();
    m_VerticalScrollbarGeometry = FGeometry();
    m_HorizontalThumbGeometry = FGeometry();
    m_VerticalThumbGeometry = FGeometry();

    if (m_bShowHorizontalScrollbar) {
        m_HorizontalScrollbarGeometry = FGeometry(
            FVector2(
                innerGeometry.Position.X,
                innerGeometry.Position.Y + viewportSize.Y + scrollbarPadding),
            FVector2(viewportSize.X, scrollbarThickness));

        const float trackWidth = m_HorizontalScrollbarGeometry.Size.X;
        const float thumbWidth = std::min(
            trackWidth,
            std::max(
                std::min(trackWidth, m_Style.ThumbMinLength),
                m_CachedContentSize.X > 0.0f ? trackWidth * (viewportSize.X / m_CachedContentSize.X) : trackWidth));
        const float availableTrack = std::max(0.0f, trackWidth - thumbWidth);
        const float thumbOffset = m_MaxScrollOffset.X > 0.0f
            ? (m_ScrollOffset.X / m_MaxScrollOffset.X) * availableTrack
            : 0.0f;
        m_HorizontalThumbGeometry = FGeometry(
            FVector2(
                m_HorizontalScrollbarGeometry.Position.X + thumbOffset,
                m_HorizontalScrollbarGeometry.Position.Y),
            FVector2(thumbWidth, scrollbarThickness));
    }

    if (m_bShowVerticalScrollbar) {
        m_VerticalScrollbarGeometry = FGeometry(
            FVector2(
                innerGeometry.Position.X + viewportSize.X + scrollbarPadding,
                innerGeometry.Position.Y),
            FVector2(scrollbarThickness, viewportSize.Y));

        const float trackHeight = m_VerticalScrollbarGeometry.Size.Y;
        const float thumbHeight = std::min(
            trackHeight,
            std::max(
                std::min(trackHeight, m_Style.ThumbMinLength),
                m_CachedContentSize.Y > 0.0f ? trackHeight * (viewportSize.Y / m_CachedContentSize.Y) : trackHeight));
        const float availableTrack = std::max(0.0f, trackHeight - thumbHeight);
        const float thumbOffset = m_MaxScrollOffset.Y > 0.0f
            ? (m_ScrollOffset.Y / m_MaxScrollOffset.Y) * availableTrack
            : 0.0f;
        m_VerticalThumbGeometry = FGeometry(
            FVector2(
                m_VerticalScrollbarGeometry.Position.X,
                m_VerticalScrollbarGeometry.Position.Y + thumbOffset),
            FVector2(scrollbarThickness, thumbHeight));
    }
}

void ImScrollBox::ClampScrollOffset()
{
    m_ScrollOffset.X = std::clamp(m_ScrollOffset.X, 0.0f, m_MaxScrollOffset.X);
    m_ScrollOffset.Y = std::clamp(m_ScrollOffset.Y, 0.0f, m_MaxScrollOffset.Y);
}

void ImScrollBox::UpdateHoveredScrollbar(const FVector2& cursorPosition)
{
    EHoveredScrollbar nextHovered = EHoveredScrollbar::None;
    if (m_bShowHorizontalScrollbar && m_HorizontalThumbGeometry.Contains(cursorPosition)) {
        nextHovered = EHoveredScrollbar::Horizontal;
    } else if (m_bShowVerticalScrollbar && m_VerticalThumbGeometry.Contains(cursorPosition)) {
        nextHovered = EHoveredScrollbar::Vertical;
    }

    if (m_HoveredScrollbar == nextHovered) {
        return;
    }

    m_HoveredScrollbar = nextHovered;
    Invalidate(EInvalidateReason::Paint);
}

void ImScrollBox::BeginScrollbarDrag(EActiveScrollbar activeScrollbar, float grabOffset)
{
    m_ActiveScrollbar = activeScrollbar;
    m_ActiveGrabOffset = std::max(0.0f, grabOffset);
    Invalidate(EInvalidateReason::Paint);
}

void ImScrollBox::UpdateScrollbarDrag(const FVector2& cursorPosition)
{
    if (m_ActiveScrollbar == EActiveScrollbar::None) {
        return;
    }

    FVector2 nextOffset = m_ScrollOffset;

    if (m_ActiveScrollbar == EActiveScrollbar::Horizontal && m_HorizontalScrollbarGeometry.Size.X > 0.0f) {
        const float trackWidth = m_HorizontalScrollbarGeometry.Size.X;
        const float thumbWidth = m_HorizontalThumbGeometry.Size.X;
        const float availableTrack = std::max(0.0f, trackWidth - thumbWidth);
        if (availableTrack <= 0.0f || m_MaxScrollOffset.X <= 0.0f) {
            nextOffset.X = 0.0f;
        } else {
            const float thumbPosition = std::clamp(
                cursorPosition.X - m_HorizontalScrollbarGeometry.Position.X - m_ActiveGrabOffset,
                0.0f,
                availableTrack);
            nextOffset.X = (thumbPosition / availableTrack) * m_MaxScrollOffset.X;
        }
    } else if (m_ActiveScrollbar == EActiveScrollbar::Vertical && m_VerticalScrollbarGeometry.Size.Y > 0.0f) {
        const float trackHeight = m_VerticalScrollbarGeometry.Size.Y;
        const float thumbHeight = m_VerticalThumbGeometry.Size.Y;
        const float availableTrack = std::max(0.0f, trackHeight - thumbHeight);
        if (availableTrack <= 0.0f || m_MaxScrollOffset.Y <= 0.0f) {
            nextOffset.Y = 0.0f;
        } else {
            const float thumbPosition = std::clamp(
                cursorPosition.Y - m_VerticalScrollbarGeometry.Position.Y - m_ActiveGrabOffset,
                0.0f,
                availableTrack);
            nextOffset.Y = (thumbPosition / availableTrack) * m_MaxScrollOffset.Y;
        }
    }

    SetScrollOffset(nextOffset);
}

void ImScrollBox::EndScrollbarDrag()
{
    if (m_ActiveScrollbar == EActiveScrollbar::None) {
        return;
    }

    m_ActiveScrollbar = EActiveScrollbar::None;
    m_ActiveGrabOffset = 0.0f;
    Invalidate(EInvalidateReason::Paint);
}

bool ImScrollBox::IsDescendantOfContent(const std::shared_ptr<ImWidget>& widget) const
{
    if (!widget || !m_Content) {
        return false;
    }

    for (std::shared_ptr<ImWidget> current = widget; current; current = current->GetParent()) {
        if (current == m_Content) {
            return true;
        }
    }

    return false;
}

} // namespace ImWidgetV4
