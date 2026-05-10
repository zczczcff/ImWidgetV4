#include <imwidgetv4/widgets/TextOutlineView.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imgui.h>
#include <algorithm>
#include <cfloat>

namespace ImWidgetV4 {

namespace {

constexpr int GInvalidVisibleIndex = -1;

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

} // namespace

ImTextOutlineView::ImTextOutlineView()
    : ImWidget()
{
    SetHitTestVisible(true);
    SetSupportsKeyboardFocus(true);
}

ImTextOutlineItem* ImTextOutlineView::AddRootItem(const std::string& text)
{
    auto item = std::make_unique<ImTextOutlineItem>();
    item->Text = text;
    ImTextOutlineItem* itemPtr = item.get();
    RootItems_.push_back(std::move(item));
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return itemPtr;
}

ImTextOutlineItem* ImTextOutlineView::AddChildItem(ImTextOutlineItem* parent, const std::string& text)
{
    if (parent == nullptr || !ContainsItem(parent)) {
        return nullptr;
    }

    auto item = std::make_unique<ImTextOutlineItem>();
    item->Text = text;
    item->Parent = parent;
    ImTextOutlineItem* itemPtr = item.get();
    parent->Children.push_back(std::move(item));
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    return itemPtr;
}

void ImTextOutlineView::RemoveItem(ImTextOutlineItem* item)
{
    if (item == nullptr) {
        return;
    }

    if (SelectedItem_ != nullptr && IsDescendantOf(SelectedItem_, item)) {
        SelectedItem_ = nullptr;
        OnSelectionChanged.Broadcast(*this, nullptr);
    }

    const bool removed = RemoveItemRecursive(RootItems_, item);
    if (!removed) {
        return;
    }

    if (HoveredItem_ != nullptr && (HoveredItem_ == item || IsDescendantOf(HoveredItem_, item))) {
        HoveredItem_ = nullptr;
    }
    if (PressedItem_ != nullptr && (PressedItem_ == item || IsDescendantOf(PressedItem_, item))) {
        PressedItem_ = nullptr;
    }
    if (DraggedItem_ != nullptr && (DraggedItem_ == item || IsDescendantOf(DraggedItem_, item))) {
        DraggedItem_ = nullptr;
    }
    if (DropTarget_.Item != nullptr && (DropTarget_.Item == item || IsDescendantOf(DropTarget_.Item, item))) {
        DropTarget_ = FDropTargetState();
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextOutlineView::ClearItems()
{
    if (RootItems_.empty()) {
        return;
    }

    RootItems_.clear();
    VisibleEntries_.clear();
    SelectedItem_ = nullptr;
    HoveredItem_ = nullptr;
    PressedItem_ = nullptr;
    DraggedItem_ = nullptr;
    DropTarget_ = FDropTargetState();
    ContentHeight_ = 0.0f;
    ScrollOffsetY_ = 0.0f;
    MaxScrollOffsetY_ = 0.0f;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextOutlineView::SetSelectedItem(ImTextOutlineItem* item)
{
    SetSelectedItemInternal(item, true, true);
}

void ImTextOutlineView::ClearSelection()
{
    SetSelectedItemInternal(nullptr, true, false);
}

bool ImTextOutlineView::ScrollToItem(ImTextOutlineItem* item, bool bCenterIfLarger)
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

void ImTextOutlineView::ExpandAll()
{
    for (const std::unique_ptr<ImTextOutlineItem>& item : RootItems_) {
        if (item) {
            ExpandAllRecursive(*item, true);
        }
    }

    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextOutlineView::CollapseAll()
{
    for (const std::unique_ptr<ImTextOutlineItem>& item : RootItems_) {
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

void ImTextOutlineView::SetStyle(const FTextOutlineViewStyle& style)
{
    Style_ = style;
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
}

void ImTextOutlineView::SetScrollOffset(float offset)
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

void ImTextOutlineView::Paint(const FPaintContext& paintContext)
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
    for (const FVisibleEntry& entry : VisibleEntries_) {
        if (entry.RowGeometry.GetMax().Y < ViewportGeometry_.Position.Y ||
            entry.RowGeometry.Position.Y > ViewportGeometry_.GetMax().Y) {
            continue;
        }

        FColor rowColor = FColor::Transparent;
        if (entry.Item == SelectedItem_) {
            rowColor = HasKeyboardFocus() ? Style_.SelectedFocusedRowColor : Style_.SelectedRowColor;
        } else if (entry.Item == DropTarget_.Item && DropTarget_.Zone == ETextOutlineDropZone::OnItem) {
            rowColor = Style_.SelectedRowColor.Lerp(Style_.SelectedFocusedRowColor, 0.35f);
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

        if (!entry.Item->IconBrush.IsValid() &&
            entry.Item->IconType >= 0 &&
            GetApplication() != nullptr) {
            entry.Item->IconBrush =
                GetApplication()->GetCoreIconBrush(static_cast<ECoreIcon>(entry.Item->IconType), Style_.TextColor);
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

        paintContext.DrawContext_.PushClipRect(entry.ContentGeometry.GetMin(), entry.ContentGeometry.GetMax(), true);
        FVector2 textPosition = entry.ContentGeometry.Position;
        if (entry.Item->IconBrush.IsValid()) {
            const float iconSize = std::min(
                Style_.IconSize,
                std::max(0.0f, entry.ContentGeometry.Size.Y));
            const FVector2 iconMin(
                entry.ContentGeometry.Position.X,
                entry.ContentGeometry.Position.Y + (entry.ContentGeometry.Size.Y - iconSize) * 0.5f);
            const FVector2 iconMax(iconMin.X + iconSize, iconMin.Y + iconSize);
            ImTextureID textureId = entry.Item->IconBrush.TextureId;
            if (GetApplication() != nullptr) {
                textureId = GetApplication()->ResolveTextureForPaint(textureId);
            }
            if (textureId != nullptr) {
                paintContext.DrawContext_.DrawImage(
                    textureId,
                    iconMin,
                    iconMax,
                    entry.Item->IconBrush.Uv0,
                    entry.Item->IconBrush.Uv1,
                    entry.Item->IconBrush.TintColor);
                textPosition.X += iconSize + Style_.IconSpacing;
            }
        }
        paintContext.DrawContext_.DrawText(
            textPosition,
            Style_.TextColor,
            entry.Item->Text,
            Style_.FontSize);
        paintContext.DrawContext_.PopClipRect();

        if (entry.Item == DropTarget_.Item && DropTarget_.Zone != ETextOutlineDropZone::OnItem) {
            const FGeometry indicatorGeometry = ResolveDropIndicatorGeometry(entry, DropTarget_.Zone);
            paintContext.DrawContext_.DrawRectFilled(
                indicatorGeometry.GetMin(),
                indicatorGeometry.GetMax(),
                Style_.SelectedFocusedRowColor,
                0.0f);
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

FVector2 ImTextOutlineView::GetMinSize() const
{
    return Style_.MinDesiredSize;
}

FReply ImTextOutlineView::OnInputEvent(const FInputEvent& event)
{
    Relayout();

    switch (event.Type) {
    case EInputEventType::MouseButtonDown: {
        if (!m_Geometry.Contains(event.MousePosition)) {
            return FReply::Unhandled();
        }

        if (event.MouseButton == EMouseButton::Left) {
            if (VerticalThumbGeometry_.IsValid() && VerticalThumbGeometry_.Contains(event.MousePosition)) {
                BeginScrollbarDrag(event.MousePosition.Y - VerticalThumbGeometry_.Position.Y);
                return FReply::Handled()
                    .SetKeyboardFocus(shared_from_this())
                    .CaptureMouse(shared_from_this(), EMouseButton::Left);
            }

            FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
            if (entry == nullptr) {
                return FReply::Unhandled();
            }

            if (entry->IndicatorGeometry.IsValid() &&
                entry->IndicatorGeometry.Contains(event.MousePosition) &&
                !entry->Item->Children.empty()) {
                PressedItem_ = nullptr;
                SetExpandedState(entry->Item, !entry->Item->Expanded, true);
                SetSelectedItemInternal(entry->Item, true, false);
                return FReply::Handled().SetKeyboardFocus(shared_from_this());
            }

            PressedItem_ = entry->Item;
            SetSelectedItemInternal(entry->Item, true, true);
            return FReply::Handled()
                .SetKeyboardFocus(shared_from_this())
                .DetectDrag(shared_from_this(), EMouseButton::Left);
        }

        if (event.MouseButton == EMouseButton::Right) {
            FVisibleEntry* entry = ResolveEntryAt(event.MousePosition);
            if (entry == nullptr) {
                return FReply::Unhandled();
            }

            SetSelectedItemInternal(entry->Item, true, true);
            OnItemContextMenuRequested.Broadcast(*this, *entry->Item, event.MousePosition);
            return FReply::Handled().SetKeyboardFocus(shared_from_this());
        }

        return FReply::Unhandled();
    }

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
        if (event.MouseButton == EMouseButton::Left) {
            PressedItem_ = nullptr;
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
        if (event.Key == EKey::DeleteKey) {
            OnDeleteRequested.Broadcast(*this, SelectedItem_);
            return FReply::Handled();
        }
        HandleKeyboardNavigation(event.Key);
        return FReply::Handled();

    default:
        return FReply::Unhandled();
    }
}

std::shared_ptr<FDragDropOperation> ImTextOutlineView::OnDragDetected(const FDragDetectEvent&)
{
    if (PressedItem_ == nullptr || !ContainsItem(PressedItem_)) {
        return nullptr;
    }

    DraggedItem_ = PressedItem_;
    DropTarget_ = FDropTargetState();

    std::shared_ptr<FDragDropOperation> operation;
    OnItemDragDetected.Broadcast(*this, *PressedItem_, operation);
    if (!operation || !operation->IsValid()) {
        DraggedItem_ = nullptr;
        return nullptr;
    }

    Invalidate(EInvalidateReason::Paint);
    return operation;
}

FReply ImTextOutlineView::OnDragEvent(const FDragDropEvent& event)
{
    switch (event.Type) {
    case EDragDropEventType::DragEnter:
    case EDragDropEventType::DragOver: {
        FVisibleEntry* entry = ResolveEntryAt(event.CurrentPosition);
        ImTextOutlineItem* candidate = entry != nullptr ? entry->Item : nullptr;
        const ETextOutlineDropZone zone =
            entry != nullptr ? ResolveDropZone(*entry, event.CurrentPosition) : ETextOutlineDropZone::OnItem;
        bool bAccepted = false;
        if (candidate != nullptr) {
            OnItemDropTest.Broadcast(*this, *candidate, zone, event.Operation, event.CurrentPosition, bAccepted);
        }

        const FDropTargetState nextDropTarget {candidate, zone};
        if (DropTarget_ != nextDropTarget) {
            DropTarget_ = nextDropTarget;
            Invalidate(EInvalidateReason::Paint);
        }

        if (candidate != nullptr && bAccepted) {
            return FReply::Handled();
        }

        return FReply::Unhandled();
    }

    case EDragDropEventType::DragLeave:
        if (DropTarget_.Item != nullptr) {
            DropTarget_ = FDropTargetState();
            Invalidate(EInvalidateReason::Paint);
        }
        return FReply::Unhandled();

    case EDragDropEventType::Drop: {
        FVisibleEntry* entry = ResolveEntryAt(event.CurrentPosition);
        ImTextOutlineItem* candidate = entry != nullptr ? entry->Item : DropTarget_.Item;
        const ETextOutlineDropZone zone =
            entry != nullptr ? ResolveDropZone(*entry, event.CurrentPosition) : DropTarget_.Zone;
        bool bHandled = false;
        if (candidate != nullptr) {
            DropTarget_ = FDropTargetState {candidate, zone};
            OnItemDropped.Broadcast(*this, *candidate, zone, event.Operation, event.CurrentPosition, bHandled);
        }

        DropTarget_ = FDropTargetState();
        Invalidate(EInvalidateReason::Paint);
        return bHandled ? FReply::Handled() : FReply::Unhandled();
    }

    case EDragDropEventType::DragEnd:
        DraggedItem_ = nullptr;
        DropTarget_ = FDropTargetState();
        PressedItem_ = nullptr;
        Invalidate(EInvalidateReason::Paint);
        return FReply::Unhandled();

    default:
        return FReply::Unhandled();
    }
}

bool ImTextOutlineView::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath)
{
    if (!m_bHitTestVisible || !m_bVisible || !m_Geometry.Contains(position)) {
        return false;
    }

    Relayout();
    outPath.push_back(shared_from_this());
    return true;
}

void ImTextOutlineView::OnFocusChanged(bool)
{
    Invalidate(EInvalidateReason::Paint);
}

void ImTextOutlineView::Relayout()
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

void ImTextOutlineView::RebuildVisibleEntries(const FGeometry& viewportGeometry)
{
    VisibleEntries_.clear();
    ViewportGeometry_ = viewportGeometry;

    float cursorY = 0.0f;
    for (const std::unique_ptr<ImTextOutlineItem>& item : RootItems_) {
        if (item) {
            FlattenVisibleChildren(*item, 0, cursorY);
        }
    }

    ContentHeight_ = std::max(0.0f, cursorY);
    UpdateVisibleEntryGeometries();
    bLayoutDirty_ = false;
}

void ImTextOutlineView::ClampScrollOffset()
{
    ScrollOffsetY_ = std::clamp(ScrollOffsetY_, 0.0f, MaxScrollOffsetY_);
    UpdateVisibleEntryGeometries();
}

void ImTextOutlineView::UpdateVisibleEntryGeometries()
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
    }
}

void ImTextOutlineView::SetExpandedState(ImTextOutlineItem* item, bool expanded, bool bBroadcast)
{
    if (item == nullptr || item->Expanded == expanded) {
        return;
    }

    item->Expanded = expanded;
    if (!expanded) {
        SelectFallbackForCollapsedItem(item);
    }
    bLayoutDirty_ = true;
    Invalidate(EInvalidateReason::Layout | EInvalidateReason::Paint);
    if (bBroadcast) {
        OnItemExpandedChanged.Broadcast(*this, *item, expanded);
    }
}

void ImTextOutlineView::SelectFallbackForCollapsedItem(ImTextOutlineItem* item)
{
    if (item == nullptr || SelectedItem_ == nullptr || SelectedItem_ == item) {
        return;
    }

    if (IsDescendantOf(SelectedItem_, item)) {
        SetSelectedItemInternal(item, true, false);
    }
}

void ImTextOutlineView::SetSelectedItemInternal(ImTextOutlineItem* item, bool bBroadcast, bool bEnsureVisible)
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

void ImTextOutlineView::EnsureAncestorsExpanded(ImTextOutlineItem* item, bool bBroadcast)
{
    ImTextOutlineItem* current = item != nullptr ? item->Parent : nullptr;
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

void ImTextOutlineView::ExpandAllRecursive(ImTextOutlineItem& item, bool expanded)
{
    item.Expanded = expanded;
    for (const std::unique_ptr<ImTextOutlineItem>& child : item.Children) {
        if (child) {
            ExpandAllRecursive(*child, expanded);
        }
    }
}

bool ImTextOutlineView::ContainsItem(const ImTextOutlineItem* item) const
{
    if (item == nullptr) {
        return false;
    }

    const ImTextOutlineItem* current = item;
    while (current->Parent != nullptr) {
        current = current->Parent;
    }

    for (const std::unique_ptr<ImTextOutlineItem>& rootItem : RootItems_) {
        if (rootItem.get() == current) {
            return true;
        }
    }

    return false;
}

bool ImTextOutlineView::IsDescendantOf(const ImTextOutlineItem* item, const ImTextOutlineItem* ancestor) const
{
    const ImTextOutlineItem* current = item;
    while (current != nullptr) {
        if (current == ancestor) {
            return true;
        }
        current = current->Parent;
    }
    return false;
}

ImTextOutlineItem* ImTextOutlineView::GetRootAncestor(ImTextOutlineItem* item) const
{
    if (item == nullptr) {
        return nullptr;
    }

    ImTextOutlineItem* current = item;
    while (current->Parent != nullptr) {
        current = current->Parent;
    }
    return current;
}

ImTextOutlineItem* ImTextOutlineView::GetParentItem(ImTextOutlineItem* item) const
{
    return item != nullptr ? item->Parent : nullptr;
}

ImTextOutlineView::FVisibleEntry* ImTextOutlineView::FindVisibleEntry(ImTextOutlineItem* item)
{
    auto it = std::find_if(
        VisibleEntries_.begin(),
        VisibleEntries_.end(),
        [item](const FVisibleEntry& entry) {
            return entry.Item == item;
        });
    return it != VisibleEntries_.end() ? &(*it) : nullptr;
}

const ImTextOutlineView::FVisibleEntry* ImTextOutlineView::FindVisibleEntry(ImTextOutlineItem* item) const
{
    auto it = std::find_if(
        VisibleEntries_.begin(),
        VisibleEntries_.end(),
        [item](const FVisibleEntry& entry) {
            return entry.Item == item;
        });
    return it != VisibleEntries_.end() ? &(*it) : nullptr;
}

int ImTextOutlineView::FindVisibleIndex(ImTextOutlineItem* item) const
{
    for (std::size_t index = 0; index < VisibleEntries_.size(); ++index) {
        if (VisibleEntries_[index].Item == item) {
            return static_cast<int>(index);
        }
    }
    return GInvalidVisibleIndex;
}

ImTextOutlineItem* ImTextOutlineView::ResolveItemAt(const FVector2& position)
{
    FVisibleEntry* entry = ResolveEntryAt(position);
    return entry != nullptr ? entry->Item : nullptr;
}

ImTextOutlineView::FVisibleEntry* ImTextOutlineView::ResolveEntryAt(const FVector2& position)
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

bool ImTextOutlineView::RemoveItemRecursive(std::vector<std::unique_ptr<ImTextOutlineItem>>& items, ImTextOutlineItem* target)
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

void ImTextOutlineView::BeginScrollbarDrag(float grabOffset)
{
    bDraggingScrollbar_ = true;
    ActiveGrabOffset_ = std::max(0.0f, grabOffset);
    Invalidate(EInvalidateReason::Paint);
}

void ImTextOutlineView::UpdateScrollbarDrag(const FVector2& cursorPosition)
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

void ImTextOutlineView::EndScrollbarDrag()
{
    bDraggingScrollbar_ = false;
    ActiveGrabOffset_ = 0.0f;
    Invalidate(EInvalidateReason::Paint);
}

void ImTextOutlineView::HandleKeyboardNavigation(EKey key)
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

void ImTextOutlineView::FlattenVisibleChildren(ImTextOutlineItem& item, int depth, float& cursorY)
{
    const float rowHeight = std::max(Style_.RowHeight, Style_.FontSize + Style_.RowPadding.Top + Style_.RowPadding.Bottom);
    FVisibleEntry entry;
    entry.Item = &item;
    entry.Depth = depth;
    entry.TextWidth = MeasureTextWidth(item.Text) +
        (item.IconBrush.IsValid() ? (Style_.IconSize + Style_.IconSpacing) : 0.0f);
    entry.ContentY = cursorY;
    entry.RowHeight = rowHeight;
    VisibleEntries_.push_back(entry);
    cursorY += rowHeight;

    if (item.Expanded) {
        for (const std::unique_ptr<ImTextOutlineItem>& child : item.Children) {
            if (child) {
                FlattenVisibleChildren(*child, depth + 1, cursorY);
            }
        }
    }
}

float ImTextOutlineView::MeasureTextWidth(const std::string& text) const
{
    if (text.empty()) {
        return 0.0f;
    }

    if (ImGui::GetCurrentContext() != nullptr && ImGui::GetFont() != nullptr) {
        return ImGui::GetFont()->CalcTextSizeA(Style_.FontSize, FLT_MAX, 0.0f, text.c_str()).x;
    }

    return Style_.FontSize * 0.55f * static_cast<float>(text.size());
}

ETextOutlineDropZone ImTextOutlineView::ResolveDropZone(const FVisibleEntry& entry, const FVector2& position) const
{
    const float localY = position.Y - entry.RowGeometry.Position.Y;
    const float topThreshold = entry.RowGeometry.Size.Y * 0.25f;
    const float bottomThreshold = entry.RowGeometry.Size.Y * 0.75f;
    if (localY <= topThreshold) {
        return ETextOutlineDropZone::BeforeItem;
    }
    if (localY >= bottomThreshold) {
        return ETextOutlineDropZone::AfterItem;
    }
    return ETextOutlineDropZone::OnItem;
}

FGeometry ImTextOutlineView::ResolveDropIndicatorGeometry(const FVisibleEntry& entry, ETextOutlineDropZone zone) const
{
    const float lineHeight = 2.0f;
    float y = entry.RowGeometry.Position.Y;
    if (zone == ETextOutlineDropZone::AfterItem) {
        y = entry.RowGeometry.GetMax().Y - lineHeight;
    }
    return FGeometry(
        FVector2(entry.RowGeometry.Position.X, y),
        FVector2(entry.RowGeometry.Size.X, lineHeight));
}

} // namespace ImWidgetV4
