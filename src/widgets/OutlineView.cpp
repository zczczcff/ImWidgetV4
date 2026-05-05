#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>

namespace ImWidgetV4 {

namespace {

constexpr int GInvalidVisibleIndex = -1;

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

} // namespace

ImOutlineView::ImOutlineView()
    : ImWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

ImOutlineItem* ImOutlineView::AddRootItem(const std::shared_ptr<ImWidget>& content)
{
    if (!content) {
        return nullptr;
    }

    auto item = std::make_unique<ImOutlineItem>();
    item->ContentWidget = content;
    ImOutlineItem* itemPtr = item.get();
    RegisterContentWidget(content);
    RootItems_.push_back(std::move(item));
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return itemPtr;
}

ImOutlineItem* ImOutlineView::AddChildItem(ImOutlineItem* parent, const std::shared_ptr<ImWidget>& content)
{
    if (parent == nullptr || content == nullptr || !ContainsItem(parent)) {
        return nullptr;
    }

    auto item = std::make_unique<ImOutlineItem>();
    item->Parent = parent;
    item->ContentWidget = content;
    ImOutlineItem* itemPtr = item.get();
    RegisterContentWidget(content);
    parent->Children.push_back(std::move(item));
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return itemPtr;
}

void ImOutlineView::RemoveItem(ImOutlineItem* item)
{
    if (item == nullptr) {
        return;
    }

    CleanupInteractionStateForItemSubtree(*item);
    if (SelectedItem_ != nullptr && IsDescendantOf(SelectedItem_, item)) {
        SelectedItem_ = nullptr;
        OnSelectionChanged.Broadcast(*this, nullptr);
    }

    const bool removed = RemoveItemRecursive(RootItems_, item);
    if (!removed) {
        return;
    }
    RefreshRegisteredContentWidgets();

    if (HoveredItem_ != nullptr && (HoveredItem_ == item || IsDescendantOf(HoveredItem_, item))) {
        HoveredItem_ = nullptr;
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImOutlineView::ClearItems()
{
    if (RootItems_.empty()) {
        return;
    }

    for (std::unique_ptr<ImOutlineItem>& item : RootItems_) {
        if (item) {
            CleanupInteractionStateForItemSubtree(*item);
        }
    }

    ImWidget::ClearChildren();
    RootItems_.clear();
    VisibleEntries_.clear();
    SelectedItem_ = nullptr;
    HoveredItem_ = nullptr;
    ContentHeight_ = 0.0f;
    ScrollOffsetY_ = 0.0f;
    MaxScrollOffsetY_ = 0.0f;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImOutlineView::SetSelectedItem(ImOutlineItem* item)
{
    SetSelectedItemInternal(item, true, true);
}

void ImOutlineView::ClearSelection()
{
    SetSelectedItemInternal(nullptr, true, false);
}

bool ImOutlineView::ScrollToItem(ImOutlineItem* item, bool bCenterIfLarger)
{
    if (item == nullptr || !ContainsItem(item)) {
        return false;
    }

    EnsureAncestorsExpanded(item, false);
    Relayout();
    const FVisibleEntry* entry = FindVisibleEntry(item);
    if (entry == nullptr || !ViewportGeometry_.IsValid()) {
        return false;
    }

    const float nextOffset = ResolveVisibleOffset(
        entry->RowGeometry.Position.Y - ViewportGeometry_.Position.Y + ScrollOffsetY_,
        entry->RowGeometry.Size.Y,
        ViewportGeometry_.Size.Y,
        ScrollOffsetY_,
        bCenterIfLarger);
    SetScrollOffset(nextOffset);
    return true;
}

void ImOutlineView::ExpandAll()
{
    for (const std::unique_ptr<ImOutlineItem>& item : RootItems_) {
        if (item) {
            ExpandAllRecursive(*item, true);
        }
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImOutlineView::CollapseAll()
{
    for (const std::unique_ptr<ImOutlineItem>& item : RootItems_) {
        if (item) {
            ExpandAllRecursive(*item, false);
        }
    }

    if (SelectedItem_ != nullptr) {
        SetSelectedItemInternal(GetRootAncestor(SelectedItem_), true, false);
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImOutlineView::SetStyle(const FOutlineViewStyle& style)
{
    Style_ = style;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImOutlineView::SetScrollOffset(float offset)
{
    Relayout();
    const float clampedOffset = std::clamp(offset, 0.0f, MaxScrollOffsetY_);
    if (ScrollOffsetY_ == clampedOffset) {
        return;
    }

    ScrollOffsetY_ = clampedOffset;
    UpdateVisibleEntryGeometries();
    Invalidate(EInvalidateReason::Paint);
}

void ImOutlineView::Paint(const FPaintContext& paintContext)
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
        Style_.BorderColor,
        Style_.CornerRadius,
        Style_.BorderThickness);

    if (HasKeyboardFocus()) {
        paintContext.DrawContext_.DrawRect(
            m_Geometry.GetMin() + FVector2(2.0f, 2.0f),
            m_Geometry.GetMax() - FVector2(2.0f, 2.0f),
            Style_.FocusedOutlineColor,
            std::max(0.0f, Style_.CornerRadius - 2.0f),
            1.5f);
    }

    paintContext.DrawContext_.PushClipRect(ViewportGeometry_.GetMin(), ViewportGeometry_.GetMax(), true);
    for (const FVisibleEntry& entry : VisibleEntries_) {
        if (entry.RowGeometry.GetMax().Y < ViewportGeometry_.Position.Y ||
            entry.RowGeometry.Position.Y > ViewportGeometry_.GetMax().Y) {
            continue;
        }

        FColor rowColor = FColor::Transparent;
        if (entry.Item == SelectedItem_) {
            rowColor = HasKeyboardFocus() ? Style_.SelectedFocusedRowColor : Style_.SelectedRowColor;
        } else if (entry.Item == HoveredItem_) {
            rowColor = Style_.HoveredRowColor;
        }

        if (rowColor.A > 0.0f) {
            paintContext.DrawContext_.DrawRectFilled(
                entry.RowGeometry.GetMin(),
                entry.RowGeometry.GetMax(),
                rowColor,
                4.0f);
        }

        if (!entry.Item->Children.empty()) {
            const FVector2 center = entry.IndicatorGeometry.GetCenter();
            const float halfSize = Style_.IndicatorSize * 0.5f;
            if (entry.Item->Expanded) {
                paintContext.DrawContext_.PathLineTo(FVector2(center.X - halfSize, center.Y - halfSize * 0.4f));
                paintContext.DrawContext_.PathLineTo(FVector2(center.X + halfSize, center.Y - halfSize * 0.4f));
                paintContext.DrawContext_.PathLineTo(FVector2(center.X, center.Y + halfSize * 0.6f));
            } else {
                paintContext.DrawContext_.PathLineTo(FVector2(center.X - halfSize * 0.4f, center.Y - halfSize));
                paintContext.DrawContext_.PathLineTo(FVector2(center.X - halfSize * 0.4f, center.Y + halfSize));
                paintContext.DrawContext_.PathLineTo(FVector2(center.X + halfSize * 0.6f, center.Y));
            }
            paintContext.DrawContext_.PathFill(Style_.IndicatorColor);
        }

        if (entry.Item->ContentWidget) {
            paintContext.DrawContext_.PushClipRect(entry.ContentGeometry.GetMin(), entry.ContentGeometry.GetMax(), true);
            FPaintContext childPaintContext(
                paintContext.DrawContext_,
                entry.Item->ContentWidget->GetGeometry(),
                paintContext.StyleSet,
                paintContext.CursorPosition,
                paintContext.bHasCursorPosition,
                paintContext.DeltaTime);
            entry.Item->ContentWidget->Paint(childPaintContext);
            paintContext.DrawContext_.PopClipRect();
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
}

FVector2 ImOutlineView::GetMinSize() const
{
    return Style_.MinDesiredSize;
}

FReply ImOutlineView::OnPreviewInputEvent(const FInputEvent& event)
{
    Relayout();

    if (event.Type != EInputEventType::MouseButtonDown) {
        return FReply::Unhandled();
    }

    if (!m_Geometry.Contains(event.MousePosition)) {
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

        SetSelectedItemInternal(entry->Item, true, false);
        if (GetApplication() != nullptr) {
            GetApplication()->SetKeyboardFocus(shared_from_this());
        }

        if (entry->IndicatorGeometry.IsValid() &&
            entry->IndicatorGeometry.Contains(event.MousePosition) &&
            !entry->Item->Children.empty()) {
            SetExpandedState(entry->Item, !entry->Item->Expanded, true);
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

    if (event.MouseButton == EMouseButton::Right) {
        FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
        if (entry == nullptr) {
            return FReply::Unhandled();
        }

        SetSelectedItemInternal(entry->Item, true, false);
        if (GetApplication() != nullptr) {
            GetApplication()->SetKeyboardFocus(shared_from_this());
        }
        OnItemContextMenuRequested.Broadcast(*this, *entry->Item, event.MousePosition);
        return FReply::Handled();
    }

    return FReply::Unhandled();
}

FReply ImOutlineView::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    switch (event.Type) {
    case EInputEventType::MouseMove:
    case EInputEventType::MouseEnter:
        bHoveredScrollbar_ = VerticalThumbGeometry_.IsValid() && VerticalThumbGeometry_.Contains(event.MousePosition);
        HoveredItem_ = nullptr;
        if (!bHoveredScrollbar_) {
            FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
            HoveredItem_ = entry != nullptr ? entry->Item : nullptr;
        }
        if (bDraggingScrollbar_) {
            UpdateScrollbarDrag(event.MousePosition);
            return FReply::Handled();
        }
        Invalidate(EInvalidateReason::Paint);
        return FReply::Unhandled();

    case EInputEventType::MouseLeave:
        if (!bDraggingScrollbar_) {
            bHoveredScrollbar_ = false;
            HoveredItem_ = nullptr;
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
        if (!HasKeyboardFocus()) {
            return FReply::Unhandled();
        }
        HandleKeyboardNavigation(event.Key);
        return FReply::Handled();

    default:
        return FReply::Unhandled();
    }
}

bool ImOutlineView::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    Relayout();
    outPath.push_back(shared_from_this());

    if (VerticalScrollbarGeometry_.Contains(position)) {
        return true;
    }

    for (FVisibleEntry& entry : VisibleEntries_) {
        if (!entry.RowGeometry.Contains(position)) {
            continue;
        }

        if (entry.IndicatorGeometry.IsValid() && entry.IndicatorGeometry.Contains(position)) {
            return true;
        }

        if (entry.ContentGeometry.Contains(position) && entry.Item->ContentWidget) {
            if (entry.Item->ContentWidget->BuildHitTestPath(position, outPath)) {
                return true;
            }
        }

        return true;
    }

    return true;
}

void ImOutlineView::OnFocusChanged(bool)
{
    Invalidate(EInvalidateReason::Paint);
}

void ImOutlineView::Relayout()
{
    const FGeometry innerGeometry = InsetGeometry(m_Geometry, Style_.Padding, Style_.BorderThickness);
    if (bLayoutDirty_) {
        const FGeometry initialViewportGeometry(innerGeometry.Position, innerGeometry.Size);
        RebuildVisibleEntries(initialViewportGeometry);

        const bool bNeedScrollbar = ContentHeight_ > innerGeometry.Size.Y + 0.5f;
        const float viewportWidth = bNeedScrollbar
            ? std::max(0.0f, innerGeometry.Size.X - Style_.ScrollbarThickness - Style_.ScrollbarPadding)
            : innerGeometry.Size.X;
        const FGeometry finalViewportGeometry(
            innerGeometry.Position,
            FVector2(std::max(0.0f, viewportWidth), innerGeometry.Size.Y));

        if (bNeedScrollbar && finalViewportGeometry.Size.X != initialViewportGeometry.Size.X) {
            RebuildVisibleEntries(finalViewportGeometry);
        } else {
            ViewportGeometry_ = finalViewportGeometry;
        }
    } else {
        const bool bNeedScrollbar = ContentHeight_ > innerGeometry.Size.Y + 0.5f;
        const float viewportWidth = bNeedScrollbar
            ? std::max(0.0f, innerGeometry.Size.X - Style_.ScrollbarThickness - Style_.ScrollbarPadding)
            : innerGeometry.Size.X;
        ViewportGeometry_ = FGeometry(
            innerGeometry.Position,
            FVector2(std::max(0.0f, viewportWidth), innerGeometry.Size.Y));
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

void ImOutlineView::RebuildVisibleEntries(const FGeometry& viewportGeometry)
{
    VisibleEntries_.clear();
    ViewportGeometry_ = viewportGeometry;

    float cursorY = 0.0f;
    for (const std::unique_ptr<ImOutlineItem>& item : RootItems_) {
        if (item) {
            FlattenVisibleChildren(*item, 0, cursorY);
        }
    }

    ContentHeight_ = std::max(0.0f, cursorY);
    UpdateVisibleEntryGeometries();
    bLayoutDirty_ = false;
}

void ImOutlineView::ClampScrollOffset()
{
    ScrollOffsetY_ = std::clamp(ScrollOffsetY_, 0.0f, MaxScrollOffsetY_);
    UpdateVisibleEntryGeometries();
}

void ImOutlineView::UpdateVisibleEntryGeometries()
{
    for (FVisibleEntry& entry : VisibleEntries_) {
        const float rowY = ViewportGeometry_.Position.Y + entry.ContentY - ScrollOffsetY_;
        entry.RowGeometry = FGeometry(
            FVector2(ViewportGeometry_.Position.X, rowY),
            FVector2(ViewportGeometry_.Size.X, entry.RowHeight));

        const float depthOffset = static_cast<float>(entry.Depth) * Style_.IndentWidth;
        const float indicatorAreaX = entry.RowGeometry.Position.X + Style_.RowPadding.Left + depthOffset;
        entry.IndicatorGeometry = FGeometry();
        if (!entry.Item->Children.empty()) {
            entry.IndicatorGeometry = FGeometry(
                FVector2(indicatorAreaX, rowY + (entry.RowHeight - Style_.IndicatorSize) * 0.5f),
                FVector2(Style_.IndicatorSize, Style_.IndicatorSize));
        }

        const float contentX = indicatorAreaX +
            (!entry.Item->Children.empty() ? (Style_.IndicatorSize + Style_.IndicatorSpacing) : 0.0f);
        entry.ContentGeometry = FGeometry(
            FVector2(contentX, rowY + Style_.RowPadding.Top),
            FVector2(
                std::max(0.0f, entry.RowGeometry.Size.X - (contentX - entry.RowGeometry.Position.X) - Style_.RowPadding.Right),
                std::max(0.0f, entry.RowHeight - Style_.RowPadding.Top - Style_.RowPadding.Bottom)));
        if (entry.Item->ContentWidget) {
            entry.Item->ContentWidget->SetGeometry(entry.ContentGeometry);
        }
    }
}

void ImOutlineView::SetExpandedState(ImOutlineItem* item, bool expanded, bool bBroadcast)
{
    if (item == nullptr || item->Expanded == expanded) {
        return;
    }

    item->Expanded = expanded;
    if (!expanded) {
        CleanupInteractionStateForItemSubtree(*item);
        SelectFallbackForCollapsedItem(item);
    }
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (bBroadcast) {
        OnItemExpandedChanged.Broadcast(*this, *item, expanded);
    }
}

void ImOutlineView::SelectFallbackForCollapsedItem(ImOutlineItem* item)
{
    if (item == nullptr || SelectedItem_ == nullptr || SelectedItem_ == item) {
        return;
    }

    if (IsDescendantOf(SelectedItem_, item)) {
        SetSelectedItemInternal(item, true, false);
    }
}

void ImOutlineView::SetSelectedItemInternal(ImOutlineItem* item, bool bBroadcast, bool bEnsureVisible)
{
    if (item != nullptr && !ContainsItem(item)) {
        return;
    }

    if (item != nullptr) {
        EnsureAncestorsExpanded(item, false);
    }

    if (SelectedItem_ != item) {
        SelectedItem_ = item;
        if (bBroadcast) {
            OnSelectionChanged.Broadcast(*this, SelectedItem_);
        }
    }

    if (bEnsureVisible && SelectedItem_ != nullptr) {
        ScrollToItem(SelectedItem_, false);
    } else {
        bLayoutDirty_ = true;
        Invalidate(EInvalidateReason::Paint);
    }
}

void ImOutlineView::EnsureAncestorsExpanded(ImOutlineItem* item, bool bBroadcast)
{
    ImOutlineItem* current = item != nullptr ? item->Parent : nullptr;
    while (current != nullptr) {
        if (!current->Expanded) {
            current->Expanded = true;
            if (bBroadcast) {
                OnItemExpandedChanged.Broadcast(*this, *current, true);
            }
        }
        current = current->Parent;
    }
    bLayoutDirty_ = true;
}

void ImOutlineView::ExpandAllRecursive(ImOutlineItem& item, bool expanded)
{
    item.Expanded = expanded;
    for (const std::unique_ptr<ImOutlineItem>& child : item.Children) {
        if (child) {
            ExpandAllRecursive(*child, expanded);
        }
    }
}

bool ImOutlineView::ContainsItem(const ImOutlineItem* item) const
{
    if (item == nullptr) {
        return false;
    }

    const ImOutlineItem* current = item;
    while (current->Parent != nullptr) {
        current = current->Parent;
    }

    for (const std::unique_ptr<ImOutlineItem>& rootItem : RootItems_) {
        if (rootItem.get() == current) {
            return true;
        }
    }

    return false;
}

bool ImOutlineView::IsDescendantOf(const ImOutlineItem* item, const ImOutlineItem* ancestor) const
{
    const ImOutlineItem* current = item;
    while (current != nullptr) {
        if (current == ancestor) {
            return true;
        }
        current = current->Parent;
    }
    return false;
}

ImOutlineItem* ImOutlineView::GetRootAncestor(ImOutlineItem* item) const
{
    if (item == nullptr) {
        return nullptr;
    }

    ImOutlineItem* current = item;
    while (current->Parent != nullptr) {
        current = current->Parent;
    }
    return current;
}

ImOutlineItem* ImOutlineView::GetParentItem(ImOutlineItem* item) const
{
    return item != nullptr ? item->Parent : nullptr;
}

ImOutlineView::FVisibleEntry* ImOutlineView::FindVisibleEntry(ImOutlineItem* item)
{
    auto it = std::find_if(
        VisibleEntries_.begin(),
        VisibleEntries_.end(),
        [item](const FVisibleEntry& entry) {
            return entry.Item == item;
        });
    return it != VisibleEntries_.end() ? &(*it) : nullptr;
}

const ImOutlineView::FVisibleEntry* ImOutlineView::FindVisibleEntry(ImOutlineItem* item) const
{
    auto it = std::find_if(
        VisibleEntries_.begin(),
        VisibleEntries_.end(),
        [item](const FVisibleEntry& entry) {
            return entry.Item == item;
        });
    return it != VisibleEntries_.end() ? &(*it) : nullptr;
}

int ImOutlineView::FindVisibleIndex(ImOutlineItem* item) const
{
    for (std::size_t index = 0; index < VisibleEntries_.size(); ++index) {
        if (VisibleEntries_[index].Item == item) {
            return static_cast<int>(index);
        }
    }
    return GInvalidVisibleIndex;
}

ImOutlineItem* ImOutlineView::ResolveItemAt(const FVector2& position)
{
    FVisibleEntry* entry = ResolveEntryAt(position);
    return entry != nullptr ? entry->Item : nullptr;
}

ImOutlineView::FVisibleEntry* ImOutlineView::ResolveEntryAt(const FVector2& position)
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

bool ImOutlineView::RemoveItemRecursive(std::vector<std::unique_ptr<ImOutlineItem>>& items, ImOutlineItem* target)
{
    for (auto it = items.begin(); it != items.end(); ++it) {
        if (it->get() == target) {
            items.erase(it);
            return true;
        }

        if (RemoveItemRecursive((*it)->Children, target)) {
            return true;
        }
    }

    return false;
}

void ImOutlineView::RegisterContentWidget(const std::shared_ptr<ImWidget>& content)
{
    AddChild(content);
}

void ImOutlineView::RefreshRegisteredContentWidgets()
{
    ImWidget::ClearChildren();

    const auto registerRecursive = [this](const auto& self, ImOutlineItem& item) -> void {
        if (item.ContentWidget) {
            AddChild(item.ContentWidget);
        }

        for (const std::unique_ptr<ImOutlineItem>& child : item.Children) {
            if (child) {
                self(self, *child);
            }
        }
    };

    for (const std::unique_ptr<ImOutlineItem>& rootItem : RootItems_) {
        if (rootItem) {
            registerRecursive(registerRecursive, *rootItem);
        }
    }
}

void ImOutlineView::BeginScrollbarDrag(float grabOffset)
{
    bDraggingScrollbar_ = true;
    ActiveGrabOffset_ = std::max(0.0f, grabOffset);
    Invalidate(EInvalidateReason::Paint);
}

void ImOutlineView::UpdateScrollbarDrag(const FVector2& cursorPosition)
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

void ImOutlineView::EndScrollbarDrag()
{
    bDraggingScrollbar_ = false;
    ActiveGrabOffset_ = 0.0f;
    Invalidate(EInvalidateReason::Paint);
}

void ImOutlineView::HandleKeyboardNavigation(EKey key)
{
    Relayout();
    if (VisibleEntries_.empty()) {
        return;
    }

    if (SelectedItem_ == nullptr) {
        if (key == EKey::Down || key == EKey::Home || key == EKey::Right) {
            SetSelectedItemInternal(VisibleEntries_.front().Item, true, true);
        } else if (key == EKey::Up || key == EKey::End || key == EKey::Left) {
            SetSelectedItemInternal(VisibleEntries_.back().Item, true, true);
        }
        return;
    }

    const int selectedIndex = FindVisibleIndex(SelectedItem_);
    if (selectedIndex == GInvalidVisibleIndex) {
        SetSelectedItemInternal(GetRootAncestor(SelectedItem_), true, true);
        return;
    }

    switch (key) {
    case EKey::Up:
        if (selectedIndex > 0) {
            SetSelectedItemInternal(VisibleEntries_[static_cast<std::size_t>(selectedIndex - 1)].Item, true, true);
        }
        break;
    case EKey::Down:
        if (selectedIndex + 1 < static_cast<int>(VisibleEntries_.size())) {
            SetSelectedItemInternal(VisibleEntries_[static_cast<std::size_t>(selectedIndex + 1)].Item, true, true);
        }
        break;
    case EKey::Home:
        SetSelectedItemInternal(VisibleEntries_.front().Item, true, true);
        break;
    case EKey::End:
        SetSelectedItemInternal(VisibleEntries_.back().Item, true, true);
        break;
    case EKey::Left:
        if (SelectedItem_->Expanded && !SelectedItem_->Children.empty()) {
            SetExpandedState(SelectedItem_, false, true);
        } else if (SelectedItem_->Parent != nullptr) {
            SetSelectedItemInternal(SelectedItem_->Parent, true, true);
        }
        break;
    case EKey::Right:
        if (!SelectedItem_->Children.empty()) {
            if (!SelectedItem_->Expanded) {
                SetExpandedState(SelectedItem_, true, true);
            } else {
                SetSelectedItemInternal(SelectedItem_->Children.front().get(), true, true);
            }
        }
        break;
    default:
        break;
    }
}

void ImOutlineView::FlattenVisibleChildren(ImOutlineItem& item, int depth, float& cursorY)
{
    float contentMinHeight = 0.0f;
    if (item.ContentWidget) {
        contentMinHeight = item.ContentWidget->GetMinSize().Y;
    }

    const float rowHeight = std::max(Style_.RowMinHeight, contentMinHeight + Style_.RowPadding.Top + Style_.RowPadding.Bottom);
    FVisibleEntry entry;
    entry.Item = &item;
    entry.Depth = depth;
    entry.ContentY = cursorY;
    entry.RowHeight = rowHeight;
    VisibleEntries_.push_back(entry);
    cursorY += rowHeight;

    if (item.Expanded) {
        for (const std::unique_ptr<ImOutlineItem>& child : item.Children) {
            if (child) {
                FlattenVisibleChildren(*child, depth + 1, cursorY);
            }
        }
    }
}

void ImOutlineView::CleanupInteractionStateForItemSubtree(ImOutlineItem& item)
{
    ImApplication* application = GetApplication();
    if (application == nullptr) {
        return;
    }

    const std::shared_ptr<ImWidget>& focusedWidget = application->GetKeyboardFocus();
    if (focusedWidget && ContainsWidgetInItemSubtree(focusedWidget, item)) {
        application->ClearKeyboardFocus();
    }

    const std::shared_ptr<ImWidget>& capturedWidget = application->GetMouseCapture();
    if (capturedWidget && ContainsWidgetInItemSubtree(capturedWidget, item)) {
        application->ReleaseMouseCapture();
    }
}

bool ImOutlineView::ContainsWidgetInItemSubtree(const std::shared_ptr<ImWidget>& widget, const ImOutlineItem& item) const
{
    if (!widget) {
        return false;
    }

    const std::shared_ptr<ImWidget>& itemWidget = item.ContentWidget;
    if (itemWidget) {
        std::shared_ptr<ImWidget> current = widget;
        while (current) {
            if (current == itemWidget) {
                return true;
            }
            current = current->GetParent();
        }
    }

    for (const std::unique_ptr<ImOutlineItem>& child : item.Children) {
        if (child && ContainsWidgetInItemSubtree(widget, *child)) {
            return true;
        }
    }

    return false;
}

} // namespace ImWidgetV4
