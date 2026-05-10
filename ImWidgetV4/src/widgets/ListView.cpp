#include <imwidgetv4/widgets/ListView.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace ImWidgetV4 {

namespace {

void SetImGuiMouseCursor(ImGuiMouseCursor cursor)
{
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui::SetMouseCursor(cursor);
    }
}

FGeometry InsetGeometry(const FGeometry& geometry, const FMargin& margin, float borderThickness)
{
    const float left = borderThickness + margin.Left;
    const float top = borderThickness + margin.Top;
    const float right = borderThickness + margin.Right;
    const float bottom = borderThickness + margin.Bottom;

    return FGeometry(
        FVector2(geometry.Position.X + left, geometry.Position.Y + top),
        FVector2(
            std::max(0.0f, geometry.Size.X - left - right),
            std::max(0.0f, geometry.Size.Y - top - bottom)));
}

float ResolveVisibleOffset(float targetStart, float targetSize, float viewportSize, float currentOffset, bool bCenterIfLarger)
{
    if (viewportSize <= 0.0f) {
        return currentOffset;
    }

    if (targetSize > viewportSize) {
        return bCenterIfLarger
            ? (targetStart - (viewportSize - targetSize) * 0.5f)
            : targetStart;
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

bool IsNavigationKey(EKey key)
{
    return key == EKey::Up ||
           key == EKey::Down ||
           key == EKey::Home ||
           key == EKey::End;
}

} // namespace

ImListView::ImListView()
    : ImWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
    EnsureDefaultEmptyContent();
}

void ImListView::SetItemCount(std::size_t count)
{
    if (ItemCount_ == count) {
        return;
    }

    if (count < ItemCount_) {
        for (auto it = RealizedRows_.begin(); it != RealizedRows_.end();) {
            if (it->first >= static_cast<int>(count)) {
                CleanupInteractionStateForWidgetSubtree(it->second);
                it = RealizedRows_.erase(it);
            } else {
                ++it;
            }
        }
    }

    ItemCount_ = count;
    ReflectedItemCount_ = static_cast<int>(
        std::min<std::size_t>(count, static_cast<std::size_t>(std::numeric_limits<int>::max())));
    RowHeightCache_.resize(ItemCount_, 0.0f);

    if (SelectedIndex_ >= static_cast<int>(ItemCount_)) {
        SetSelectedIndexInternal(-1, true, false);
    }

    HoveredRowIndex_ = IsValidIndex(HoveredRowIndex_) ? HoveredRowIndex_ : -1;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImListView::SetOnGenerateRow(FOnGenerateRow callback)
{
    OnGenerateRow_ = std::move(callback);
    RequestRefresh();
}

void ImListView::RequestRefresh()
{
    ReleaseAllRealizedRows();
    std::fill(RowHeightCache_.begin(), RowHeightCache_.end(), 0.0f);

    if (SelectedIndex_ >= static_cast<int>(ItemCount_)) {
        SetSelectedIndexInternal(-1, true, false);
    }

    HoveredRowIndex_ = -1;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImListView::SetSelectedIndex(int index)
{
    SetSelectedIndexInternal(index, true, true);
}

bool ImListView::HasSelection() const
{
    return IsValidIndex(SelectedIndex_);
}

void ImListView::ClearSelection()
{
    SetSelectedIndexInternal(-1, true, false);
}

bool ImListView::ScrollToIndex(int index, bool bCenterIfLarger)
{
    if (!IsValidIndex(index)) {
        return false;
    }

    Relayout();
    const float rowTop = GetRowTop(index);
    const float rowHeight = GetRowHeightForIndex(index);
    const float nextOffset = ResolveVisibleOffset(
        rowTop,
        rowHeight,
        ViewportGeometry_.Size.Y,
        ScrollOffsetY_,
        bCenterIfLarger);
    SetScrollOffset(nextOffset);
    return true;
}

void ImListView::SetScrollOffset(float offset)
{
    Relayout();
    const float clampedOffset = std::clamp(offset, 0.0f, MaxScrollOffsetY_);
    if (ScrollOffsetY_ == clampedOffset) {
        return;
    }

    ScrollOffsetY_ = clampedOffset;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImListView::SetEmptyContent(const std::shared_ptr<ImWidget>& widget)
{
    if (EmptyContent_ == widget) {
        return;
    }

    EmptyContent_ = widget;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

std::shared_ptr<ImWidget> ImListView::GetEmptyContent() const
{
    return EmptyContent_ ? EmptyContent_ : DefaultEmptyContent_;
}

void ImListView::SetStyle(const FListViewStyle& style)
{
    Style_ = style;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImListView::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();

    paintContext.DrawContext_.DrawRectFilled(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        Style_.BackgroundColor,
        Style_.CornerRadius);
    paintContext.DrawContext_.DrawRect(
        m_Geometry.GetMin(),
        m_Geometry.GetMax(),
        HasKeyboardFocus() ? Style_.FocusedOutlineColor : Style_.BorderColor,
        Style_.CornerRadius,
        Style_.BorderThickness);

    paintContext.DrawContext_.PushClipRect(ViewportGeometry_.GetMin(), ViewportGeometry_.GetMax(), true);

    if (ItemCount_ == 0) {
        const std::shared_ptr<ImWidget> emptyContent = GetEmptyContent();
        if (emptyContent) {
            FPaintContext childPaintContext(
                paintContext.DrawContext_,
                emptyContent->GetGeometry(),
                paintContext.StyleSet,
                paintContext.CursorPosition,
                paintContext.bHasCursorPosition,
                paintContext.DeltaTime);
            emptyContent->Paint(childPaintContext);
        }
    } else {
        for (const FVisibleEntry& entry : VisibleEntries_) {
            if (entry.RowGeometry.GetMax().Y < ViewportGeometry_.Position.Y ||
                entry.RowGeometry.Position.Y > ViewportGeometry_.GetMax().Y) {
                continue;
            }

            FColor rowColor = FColor::Transparent;
            if (entry.Index == SelectedIndex_) {
                rowColor = HasKeyboardFocus() ? Style_.SelectedFocusedRowColor : Style_.SelectedRowColor;
            } else if (entry.Index == HoveredRowIndex_) {
                rowColor = Style_.HoveredRowColor;
            }

            if (rowColor.A > 0.0f) {
                paintContext.DrawContext_.DrawRectFilled(
                    entry.RowGeometry.GetMin(),
                    entry.RowGeometry.GetMax(),
                    rowColor,
                    4.0f);
            }

            if (entry.RowWidget) {
                paintContext.DrawContext_.PushClipRect(entry.ContentGeometry.GetMin(), entry.ContentGeometry.GetMax(), true);
                FPaintContext childPaintContext(
                    paintContext.DrawContext_,
                    entry.RowWidget->GetGeometry(),
                    paintContext.StyleSet,
                    paintContext.CursorPosition,
                    paintContext.bHasCursorPosition,
                    paintContext.DeltaTime);
                entry.RowWidget->Paint(childPaintContext);
                paintContext.DrawContext_.PopClipRect();
            }
        }
    }

    paintContext.DrawContext_.PopClipRect();

    if (VerticalScrollbarGeometry_.IsValid()) {
        paintContext.DrawContext_.DrawRectFilled(
            VerticalScrollbarGeometry_.GetMin(),
            VerticalScrollbarGeometry_.GetMax(),
            Style_.ScrollbarTrackColor,
            Style_.ScrollbarThickness * 0.5f);
        paintContext.DrawContext_.DrawRectFilled(
            VerticalThumbGeometry_.GetMin(),
            VerticalThumbGeometry_.GetMax(),
            (bDraggingScrollbar_ || bHoveredScrollbar_) ? Style_.ScrollbarThumbHoveredColor : Style_.ScrollbarThumbColor,
            Style_.ScrollbarThickness * 0.5f);
    }

    if (bDraggingScrollbar_ || bHoveredScrollbar_) {
        SetImGuiMouseCursor(ImGuiMouseCursor_ResizeNS);
    }
}

FVector2 ImListView::GetMinSize() const
{
    return Style_.MinDesiredSize;
}

FReply ImListView::OnPreviewInputEvent(const FInputEvent& event)
{
    Relayout();

    if (event.Type != EInputEventType::MouseButtonDown || !m_Geometry.Contains(event.MousePosition)) {
        return FReply::Unhandled();
    }

    if (event.MouseButton == EMouseButton::Left) {
        if (VerticalThumbGeometry_.IsValid() && VerticalThumbGeometry_.Contains(event.MousePosition)) {
            bHoveredScrollbar_ = true;
            BeginScrollbarDrag(event.MousePosition.Y - VerticalThumbGeometry_.Position.Y);
            return FReply::Handled()
                .SetKeyboardFocus(shared_from_this())
                .CaptureMouse(shared_from_this(), EMouseButton::Left);
        }

        FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
        if (entry == nullptr) {
            return FReply::Unhandled();
        }

        SetSelectedIndexInternal(entry->Index, true, false);
        if (GetApplication() != nullptr) {
            GetApplication()->SetKeyboardFocus(shared_from_this());
        }
        return FReply::Unhandled();
    }

    if (event.MouseButton == EMouseButton::Right) {
        FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
        if (entry == nullptr) {
            return FReply::Unhandled();
        }

        SetSelectedIndexInternal(entry->Index, true, false);
        if (GetApplication() != nullptr) {
            GetApplication()->SetKeyboardFocus(shared_from_this());
        }
        OnItemContextMenuRequested.Broadcast(*this, entry->Index, event.MousePosition);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply ImListView::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    switch (event.Type) {
    case EInputEventType::MouseMove:
    case EInputEventType::MouseEnter: {
        const bool bWasHoveredScrollbar = bHoveredScrollbar_;
        const int previousHoveredRow = HoveredRowIndex_;
        bHoveredScrollbar_ = VerticalThumbGeometry_.IsValid() && VerticalThumbGeometry_.Contains(event.MousePosition);
        HoveredRowIndex_ = -1;
        if (!bHoveredScrollbar_) {
            FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
            HoveredRowIndex_ = entry != nullptr ? entry->Index : -1;
        }

        if (bDraggingScrollbar_) {
            UpdateScrollbarDrag(event.MousePosition);
            return FReply::Handled();
        }

        if (bWasHoveredScrollbar != bHoveredScrollbar_ || previousHoveredRow != HoveredRowIndex_) {
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();
    }

    case EInputEventType::MouseLeave:
        if (!bDraggingScrollbar_ && (bHoveredScrollbar_ || HoveredRowIndex_ != -1)) {
            bHoveredScrollbar_ = false;
            HoveredRowIndex_ = -1;
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();

    case EInputEventType::MouseButtonUp:
        if (event.MouseButton == EMouseButton::Left && bDraggingScrollbar_) {
            UpdateScrollbarDrag(event.MousePosition);
            EndScrollbarDrag();
            return FReply::Handled().ReleaseMouseCapture();
        }
        return FReply::Unhandled();

    case EInputEventType::MouseWheel:
        if (!m_Geometry.Contains(event.MousePosition) || MaxScrollOffsetY_ <= 0.0f) {
            return FReply::Unhandled();
        }
        SetScrollOffset(ScrollOffsetY_ - event.ScrollDelta.Y * Style_.WheelScrollStep);
        return FReply::Handled();

    case EInputEventType::KeyDown:
        if (!HasKeyboardFocus() || !IsNavigationKey(event.Key)) {
            return FReply::Unhandled();
        }
        HandleKeyboardNavigation(event.Key);
        return FReply::Handled();

    default:
        return FReply::Unhandled();
    }
}

bool ImListView::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    Relayout();
    outPath.push_back(shared_from_this());

    if (VerticalScrollbarGeometry_.Contains(position)) {
        return true;
    }

    if (ItemCount_ == 0) {
        const std::shared_ptr<ImWidget> emptyContent = GetEmptyContent();
        if (emptyContent && emptyContent->BuildHitTestPath(position, outPath)) {
            return true;
        }
        return true;
    }

    for (FVisibleEntry& entry : VisibleEntries_) {
        if (!entry.RowGeometry.Contains(position)) {
            continue;
        }

        if (entry.ContentGeometry.Contains(position) && entry.RowWidget) {
            if (entry.RowWidget->BuildHitTestPath(position, outPath)) {
                return true;
            }
        }

        return true;
    }

    return true;
}

void ImListView::OnFocusChanged(bool)
{
    Invalidate(EInvalidateReason::Paint);
}

void ImListView::SetItemCountProperty(int& count)
{
    SetItemCount(count <= 0 ? 0U : static_cast<std::size_t>(count));
}

int& ImListView::GetItemCountProperty()
{
    ReflectedItemCount_ = static_cast<int>(
        std::min<std::size_t>(ItemCount_, static_cast<std::size_t>(std::numeric_limits<int>::max())));
    return ReflectedItemCount_;
}

void ImListView::SetSelectedIndexProperty(int& index)
{
    SetSelectedIndex(index);
}

int& ImListView::GetSelectedIndexProperty()
{
    return SelectedIndex_;
}

void ImListView::SetScrollOffsetProperty(float& offset)
{
    SetScrollOffset(offset);
}

float& ImListView::GetScrollOffsetProperty()
{
    return ScrollOffsetY_;
}

void ImListView::Relayout()
{
    const FGeometry innerGeometry = InsetGeometry(m_Geometry, Style_.Padding, Style_.BorderThickness);
    const FGeometry initialViewportGeometry(innerGeometry.Position, innerGeometry.Size);

    if (bLayoutDirty_) {
        RebuildVisibleEntries(initialViewportGeometry, true);

        const bool bNeedScrollbar = ContentHeight_ > innerGeometry.Size.Y + 0.5f;
        const float viewportWidth = bNeedScrollbar
            ? std::max(0.0f, innerGeometry.Size.X - Style_.ScrollbarThickness - Style_.ScrollbarPadding)
            : innerGeometry.Size.X;
        const FGeometry finalViewportGeometry(
            innerGeometry.Position,
            FVector2(std::max(0.0f, viewportWidth), innerGeometry.Size.Y));

        if (bNeedScrollbar && finalViewportGeometry.Size.X != initialViewportGeometry.Size.X) {
            RebuildVisibleEntries(finalViewportGeometry, true);
        } else {
            ViewportGeometry_ = finalViewportGeometry;
            UpdateVisibleEntryGeometries();
            UpdateEmptyContentGeometry();
        }
    } else {
        const bool bNeedScrollbar = ContentHeight_ > innerGeometry.Size.Y + 0.5f;
        const float viewportWidth = bNeedScrollbar
            ? std::max(0.0f, innerGeometry.Size.X - Style_.ScrollbarThickness - Style_.ScrollbarPadding)
            : innerGeometry.Size.X;
        ViewportGeometry_ = FGeometry(
            innerGeometry.Position,
            FVector2(std::max(0.0f, viewportWidth), innerGeometry.Size.Y));
        UpdateVisibleEntryGeometries();
        UpdateEmptyContentGeometry();
    }

    MaxScrollOffsetY_ = std::max(0.0f, ContentHeight_ - ViewportGeometry_.Size.Y);
    ClampScrollOffset();

    const bool bShowScrollbar = ContentHeight_ > ViewportGeometry_.Size.Y + 0.5f;
    VerticalScrollbarGeometry_ = FGeometry();
    VerticalThumbGeometry_ = FGeometry();
    if (bShowScrollbar && innerGeometry.Size.Y > 0.0f) {
        VerticalScrollbarGeometry_ = FGeometry(
            FVector2(
                ViewportGeometry_.Position.X + ViewportGeometry_.Size.X + Style_.ScrollbarPadding,
                innerGeometry.Position.Y),
            FVector2(Style_.ScrollbarThickness, innerGeometry.Size.Y));

        const float trackHeight = VerticalScrollbarGeometry_.Size.Y;
        const float thumbHeight = std::min(
            trackHeight,
            std::max(
                std::min(trackHeight, Style_.ThumbMinLength),
                ContentHeight_ > 0.0f ? trackHeight * (ViewportGeometry_.Size.Y / ContentHeight_) : trackHeight));
        const float availableTrack = std::max(0.0f, trackHeight - thumbHeight);
        const float thumbOffset = MaxScrollOffsetY_ > 0.0f
            ? (ScrollOffsetY_ / MaxScrollOffsetY_) * availableTrack
            : 0.0f;
        VerticalThumbGeometry_ = FGeometry(
            FVector2(VerticalScrollbarGeometry_.Position.X, VerticalScrollbarGeometry_.Position.Y + thumbOffset),
            FVector2(VerticalScrollbarGeometry_.Size.X, thumbHeight));
    }
}

void ImListView::RebuildVisibleEntries(const FGeometry& viewportGeometry, bool bMeasureHeights)
{
    ViewportGeometry_ = viewportGeometry;
    const float viewportHeight = std::max(0.0f, viewportGeometry.Size.Y);
    const float estimatedRowHeight = EstimateRowHeight();

    bool bAnyHeightChanged = false;
    for (int pass = 0; pass < (bMeasureHeights ? 3 : 1); ++pass) {
        VisibleEntries_.clear();
        ContentHeight_ = 0.0f;

        int firstVisibleIndex = -1;
        int lastVisibleIndex = -1;
        float cursorY = 0.0f;
        const float visibleStart = ScrollOffsetY_;
        const float visibleEnd = ScrollOffsetY_ + viewportHeight;

        for (std::size_t rawIndex = 0; rawIndex < ItemCount_; ++rawIndex) {
            const int index = static_cast<int>(rawIndex);
            const float rowHeight = GetRowHeightForIndex(index);
            const float rowStart = cursorY;
            const float rowEnd = rowStart + rowHeight;

            if (rowEnd >= visibleStart && rowStart <= visibleEnd) {
                if (firstVisibleIndex == -1) {
                    firstVisibleIndex = index;
                }
                lastVisibleIndex = index;
            } else if (firstVisibleIndex != -1 && rowStart > visibleEnd) {
                cursorY = rowEnd;
                for (std::size_t remainingIndex = rawIndex + 1; remainingIndex < ItemCount_; ++remainingIndex) {
                    cursorY += GetRowHeightForIndex(static_cast<int>(remainingIndex));
                }
                break;
            }

            cursorY = rowEnd;
        }

        ContentHeight_ = cursorY;

        if (firstVisibleIndex == -1 && ItemCount_ > 0 && viewportHeight > 0.0f) {
            firstVisibleIndex = static_cast<int>(ItemCount_ - 1);
            lastVisibleIndex = firstVisibleIndex;
        }

        const int firstRealizedIndex = firstVisibleIndex >= 0
            ? std::max(0, firstVisibleIndex - OverscanItemCount_)
            : -1;
        const int lastRealizedIndex = lastVisibleIndex >= 0
            ? std::min(static_cast<int>(ItemCount_) - 1, lastVisibleIndex + OverscanItemCount_)
            : -1;

        std::unordered_set<int> retainedIndices;
        retainedIndices.reserve(VisibleEntries_.size() + static_cast<std::size_t>(OverscanItemCount_ * 2 + 4));

        if (firstRealizedIndex >= 0 && lastRealizedIndex >= firstRealizedIndex) {
            float rowCursorY = 0.0f;
            for (std::size_t rawIndex = 0; rawIndex < ItemCount_; ++rawIndex) {
                const int index = static_cast<int>(rawIndex);
                const float rowHeight = GetRowHeightForIndex(index);
                const float rowStart = rowCursorY;
                rowCursorY += rowHeight;

                if (index < firstRealizedIndex || index > lastRealizedIndex) {
                    continue;
                }

                FVisibleEntry entry;
                entry.Index = index;
                entry.ContentY = rowStart;
                entry.RowHeight = rowHeight;
                entry.RowWidget = EnsureRealizedRow(index);
                VisibleEntries_.push_back(entry);
                retainedIndices.insert(index);
            }
        }

        for (auto it = RealizedRows_.begin(); it != RealizedRows_.end();) {
            if (retainedIndices.find(it->first) == retainedIndices.end()) {
                CleanupInteractionStateForWidgetSubtree(it->second);
                it = RealizedRows_.erase(it);
            } else {
                ++it;
            }
        }

        UpdateVisibleEntryGeometries();
        UpdateEmptyContentGeometry();

        if (!bMeasureHeights) {
            break;
        }

        bool bPassChanged = false;
        for (FVisibleEntry& entry : VisibleEntries_) {
            const float measuredHeight = entry.RowWidget
                ? std::max(Style_.RowMinHeight, entry.RowWidget->GetMinSize().Y + Style_.RowPadding.Top + Style_.RowPadding.Bottom)
                : estimatedRowHeight;
            if (std::fabs(measuredHeight - entry.RowHeight) > 0.5f) {
                RowHeightCache_[static_cast<std::size_t>(entry.Index)] = measuredHeight;
                bPassChanged = true;
                bAnyHeightChanged = true;
            }
        }

        if (!bPassChanged) {
            break;
        }
    }

    if (bAnyHeightChanged) {
        float totalHeight = 0.0f;
        for (std::size_t index = 0; index < ItemCount_; ++index) {
            totalHeight += GetRowHeightForIndex(static_cast<int>(index));
        }
        ContentHeight_ = totalHeight;
    }

    RefreshRegisteredChildren();
    bLayoutDirty_ = false;
}

void ImListView::UpdateVisibleEntryGeometries()
{
    for (FVisibleEntry& entry : VisibleEntries_) {
        const float rowY = ViewportGeometry_.Position.Y + entry.ContentY - ScrollOffsetY_;
        entry.RowGeometry = FGeometry(
            FVector2(ViewportGeometry_.Position.X, rowY),
            FVector2(ViewportGeometry_.Size.X, entry.RowHeight));
        entry.ContentGeometry = FGeometry(
            FVector2(
                entry.RowGeometry.Position.X + Style_.RowPadding.Left,
                rowY + Style_.RowPadding.Top),
            FVector2(
                std::max(0.0f, entry.RowGeometry.Size.X - Style_.RowPadding.Left - Style_.RowPadding.Right),
                std::max(0.0f, entry.RowHeight - Style_.RowPadding.Top - Style_.RowPadding.Bottom)));
        if (entry.RowWidget) {
            entry.RowWidget->SetGeometry(entry.ContentGeometry);
        }
    }
}

void ImListView::ClampScrollOffset()
{
    ScrollOffsetY_ = std::clamp(ScrollOffsetY_, 0.0f, MaxScrollOffsetY_);
    UpdateVisibleEntryGeometries();
    UpdateEmptyContentGeometry();
}

void ImListView::RefreshRegisteredChildren()
{
    ImWidget::ClearChildren();

    if (ItemCount_ == 0) {
        const std::shared_ptr<ImWidget> emptyContent = GetEmptyContent();
        if (emptyContent) {
            AddChild(emptyContent);
        }
        return;
    }

    std::sort(
        VisibleEntries_.begin(),
        VisibleEntries_.end(),
        [](const FVisibleEntry& lhs, const FVisibleEntry& rhs) {
            return lhs.Index < rhs.Index;
        });

    for (const FVisibleEntry& entry : VisibleEntries_) {
        if (entry.RowWidget) {
            AddChild(entry.RowWidget);
        }
    }
}

void ImListView::EnsureDefaultEmptyContent()
{
    if (DefaultEmptyContent_) {
        return;
    }

    auto text = std::make_shared<ImTextBlock>();
    text->SetText("No items");
    text->SetTextColor(FColor::FromBytes(164, 171, 181));
    text->SetHitTestVisible(false);
    DefaultEmptyContent_ = text;
}

void ImListView::UpdateEmptyContentGeometry()
{
    if (ItemCount_ != 0) {
        return;
    }

    const std::shared_ptr<ImWidget> emptyContent = GetEmptyContent();
    if (!emptyContent) {
        return;
    }

    const FVector2 desiredSize = emptyContent->GetMinSize();
    const FVector2 position(
        ViewportGeometry_.Position.X + std::max(0.0f, (ViewportGeometry_.Size.X - desiredSize.X) * 0.5f),
        ViewportGeometry_.Position.Y + std::max(0.0f, (ViewportGeometry_.Size.Y - desiredSize.Y) * 0.5f));
    emptyContent->SetGeometry(FGeometry(position, desiredSize));
}

void ImListView::SetSelectedIndexInternal(int index, bool bBroadcast, bool bEnsureVisible)
{
    const int normalizedIndex = IsValidIndex(index) ? index : -1;
    if (SelectedIndex_ != normalizedIndex) {
        SelectedIndex_ = normalizedIndex;
        if (bBroadcast) {
            OnSelectionChanged.Broadcast(*this, SelectedIndex_);
        }
    }

    if (bEnsureVisible && SelectedIndex_ >= 0) {
        ScrollToIndex(SelectedIndex_, false);
    } else {
        Invalidate(EInvalidateReason::Paint);
    }
}

void ImListView::HandleKeyboardNavigation(EKey key)
{
    if (ItemCount_ == 0) {
        return;
    }

    switch (key) {
    case EKey::Up:
        if (SelectedIndex_ < 0) {
            SetSelectedIndexInternal(0, true, true);
        } else if (SelectedIndex_ > 0) {
            SetSelectedIndexInternal(SelectedIndex_ - 1, true, true);
        }
        break;

    case EKey::Down:
        if (SelectedIndex_ < 0) {
            SetSelectedIndexInternal(0, true, true);
        } else if (SelectedIndex_ + 1 < static_cast<int>(ItemCount_)) {
            SetSelectedIndexInternal(SelectedIndex_ + 1, true, true);
        }
        break;

    case EKey::Home:
        SetSelectedIndexInternal(0, true, true);
        break;

    case EKey::End:
        SetSelectedIndexInternal(static_cast<int>(ItemCount_) - 1, true, true);
        break;

    default:
        break;
    }
}

void ImListView::BeginScrollbarDrag(float grabOffset)
{
    bDraggingScrollbar_ = true;
    ActiveGrabOffset_ = std::max(0.0f, grabOffset);
    Invalidate(EInvalidateReason::Paint);
}

void ImListView::UpdateScrollbarDrag(const FVector2& cursorPosition)
{
    if (!bDraggingScrollbar_ || !VerticalScrollbarGeometry_.IsValid()) {
        return;
    }

    const float availableTrack = std::max(0.0f, VerticalScrollbarGeometry_.Size.Y - VerticalThumbGeometry_.Size.Y);
    if (availableTrack <= 0.0f || MaxScrollOffsetY_ <= 0.0f) {
        SetScrollOffset(0.0f);
        return;
    }

    const float thumbPosition = std::clamp(
        cursorPosition.Y - VerticalScrollbarGeometry_.Position.Y - ActiveGrabOffset_,
        0.0f,
        availableTrack);
    SetScrollOffset((thumbPosition / availableTrack) * MaxScrollOffsetY_);
}

void ImListView::EndScrollbarDrag()
{
    bDraggingScrollbar_ = false;
    ActiveGrabOffset_ = 0.0f;
    Invalidate(EInvalidateReason::Paint);
}

ImListView::FVisibleEntry* ImListView::ResolveEntryAt(const FVector2& position)
{
    if (!ViewportGeometry_.Contains(position)) {
        return nullptr;
    }

    for (FVisibleEntry& entry : VisibleEntries_) {
        if (entry.RowGeometry.Contains(position)) {
            return &entry;
        }
    }

    return nullptr;
}

const ImListView::FVisibleEntry* ImListView::ResolveEntryAt(const FVector2& position) const
{
    if (!ViewportGeometry_.Contains(position)) {
        return nullptr;
    }

    for (const FVisibleEntry& entry : VisibleEntries_) {
        if (entry.RowGeometry.Contains(position)) {
            return &entry;
        }
    }

    return nullptr;
}

float ImListView::EstimateRowHeight() const
{
    return std::max(0.0f, Style_.RowMinHeight + Style_.RowPadding.Top + Style_.RowPadding.Bottom);
}

float ImListView::GetRowHeightForIndex(int index) const
{
    if (!IsValidIndex(index)) {
        return EstimateRowHeight();
    }

    const float cachedHeight = RowHeightCache_[static_cast<std::size_t>(index)];
    return cachedHeight > 0.0f ? cachedHeight : EstimateRowHeight();
}

float ImListView::GetRowTop(int index) const
{
    if (!IsValidIndex(index)) {
        return 0.0f;
    }

    float rowTop = 0.0f;
    for (int currentIndex = 0; currentIndex < index; ++currentIndex) {
        rowTop += GetRowHeightForIndex(currentIndex);
    }
    return rowTop;
}

bool ImListView::IsValidIndex(int index) const
{
    return index >= 0 && index < static_cast<int>(ItemCount_);
}

std::shared_ptr<ImWidget> ImListView::EnsureRealizedRow(int index)
{
    const auto existing = RealizedRows_.find(index);
    if (existing != RealizedRows_.end()) {
        return existing->second;
    }

    std::shared_ptr<ImWidget> rowWidget;
    if (OnGenerateRow_) {
        rowWidget = OnGenerateRow_(static_cast<std::size_t>(index));
    }
    if (!rowWidget) {
        rowWidget = std::make_shared<ImWidget>();
    }

    RealizedRows_[index] = rowWidget;
    return rowWidget;
}

void ImListView::ReleaseRealizedRow(int index)
{
    const auto it = RealizedRows_.find(index);
    if (it == RealizedRows_.end()) {
        return;
    }

    CleanupInteractionStateForWidgetSubtree(it->second);
    RealizedRows_.erase(it);
}

void ImListView::ReleaseAllRealizedRows()
{
    for (auto it = RealizedRows_.begin(); it != RealizedRows_.end();) {
        CleanupInteractionStateForWidgetSubtree(it->second);
        it = RealizedRows_.erase(it);
    }
    VisibleEntries_.clear();
    ImWidget::ClearChildren();
}

void ImListView::CleanupInteractionStateForWidgetSubtree(const std::shared_ptr<ImWidget>& widget)
{
    ImApplication* application = GetApplication();
    if (application == nullptr || !widget) {
        return;
    }

    const std::shared_ptr<ImWidget>& focusedWidget = application->GetKeyboardFocus();
    if (focusedWidget && ContainsWidgetInSubtree(focusedWidget, widget)) {
        application->ClearKeyboardFocus();
    }

    const std::shared_ptr<ImWidget>& capturedWidget = application->GetMouseCapture();
    if (capturedWidget && ContainsWidgetInSubtree(capturedWidget, widget)) {
        application->ReleaseMouseCapture();
    }
}

bool ImListView::ContainsWidgetInSubtree(
    const std::shared_ptr<ImWidget>& widget,
    const std::shared_ptr<ImWidget>& subtreeRoot) const
{
    if (!widget || !subtreeRoot) {
        return false;
    }

    std::shared_ptr<ImWidget> current = widget;
    while (current) {
        if (current == subtreeRoot) {
            return true;
        }
        current = current->GetParent();
    }
    return false;
}

} // namespace ImWidgetV4
