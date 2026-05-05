#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

class ImOutlineItem : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(ImOutlineItem, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Bool, "Expanded", &ImOutlineItem::Expanded, "Whether the item is expanded");
    END_DECLARE_OBJECT()

public:
    bool Expanded = false;

private:
    friend class ImOutlineView;

    ImOutlineItem* Parent = nullptr;
    std::shared_ptr<ImWidget> ContentWidget;
    std::vector<std::unique_ptr<ImOutlineItem>> Children;
};

struct FOutlineViewStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FOutlineViewStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FOutlineViewStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FOutlineViewStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FOutlineViewStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "HoveredRowColor", &FOutlineViewStyle::HoveredRowColor, "Hovered row color")
        .RegisterProperty(PropertyType::Color, "SelectedRowColor", &FOutlineViewStyle::SelectedRowColor, "Selected row color")
        .RegisterProperty(PropertyType::Color, "SelectedFocusedRowColor", &FOutlineViewStyle::SelectedFocusedRowColor, "Selected focused row color")
        .RegisterProperty(PropertyType::Color, "IndicatorColor", &FOutlineViewStyle::IndicatorColor, "Expand indicator color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FOutlineViewStyle::Padding, "Outer padding")
        .RegisterProperty(PropertyType::Struct, "RowPadding", &FOutlineViewStyle::RowPadding, "Row padding")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FOutlineViewStyle::MinDesiredSize, "Minimum desired size")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FOutlineViewStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FOutlineViewStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "IndentWidth", &FOutlineViewStyle::IndentWidth, "Indent width")
        .RegisterProperty(PropertyType::Float, "IndicatorSize", &FOutlineViewStyle::IndicatorSize, "Indicator size")
        .RegisterProperty(PropertyType::Float, "IndicatorSpacing", &FOutlineViewStyle::IndicatorSpacing, "Indicator spacing")
        .RegisterProperty(PropertyType::Float, "RowMinHeight", &FOutlineViewStyle::RowMinHeight, "Minimum row height")
        .RegisterProperty(PropertyType::Float, "ScrollbarThickness", &FOutlineViewStyle::ScrollbarThickness, "Scrollbar thickness")
        .RegisterProperty(PropertyType::Float, "ScrollbarPadding", &FOutlineViewStyle::ScrollbarPadding, "Scrollbar padding")
        .RegisterProperty(PropertyType::Color, "ScrollbarTrackColor", &FOutlineViewStyle::ScrollbarTrackColor, "Scrollbar track color")
        .RegisterProperty(PropertyType::Color, "ScrollbarThumbColor", &FOutlineViewStyle::ScrollbarThumbColor, "Scrollbar thumb color")
        .RegisterProperty(PropertyType::Color, "ScrollbarThumbHoveredColor", &FOutlineViewStyle::ScrollbarThumbHoveredColor, "Hovered scrollbar thumb color")
        .RegisterProperty(PropertyType::Float, "ThumbMinLength", &FOutlineViewStyle::ThumbMinLength, "Minimum thumb length")
        .RegisterProperty(PropertyType::Float, "WheelScrollStep", &FOutlineViewStyle::WheelScrollStep, "Wheel scroll step");
    END_DECLARE_OBJECT()

public:
    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor HoveredRowColor = FColor::FromBytes(44, 52, 63);
    FColor SelectedRowColor = FColor::FromBytes(74, 112, 168);
    FColor SelectedFocusedRowColor = FColor::FromBytes(92, 141, 214);
    FColor IndicatorColor = FColor::FromBytes(228, 232, 238);
    FMargin Padding = FMargin(8.0f);
    FMargin RowPadding = FMargin(6.0f, 8.0f, 4.0f, 4.0f);
    FVector2 MinDesiredSize {280.0f, 220.0f};
    float CornerRadius = 6.0f;
    float BorderThickness = 1.0f;
    float IndentWidth = 18.0f;
    float IndicatorSize = 10.0f;
    float IndicatorSpacing = 6.0f;
    float RowMinHeight = 28.0f;
    float ScrollbarThickness = 10.0f;
    float ScrollbarPadding = 2.0f;
    FColor ScrollbarTrackColor = FColor::FromBytes(38, 45, 56);
    FColor ScrollbarThumbColor = FColor::FromBytes(88, 102, 119);
    FColor ScrollbarThumbHoveredColor = FColor::FromBytes(122, 143, 168);
    float ThumbMinLength = 28.0f;
    float WheelScrollStep = 28.0f;
};

class ImOutlineView : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImOutlineView, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::Float,
            "ScrollOffset",
            static_cast<void (ImOutlineView::*)(float&)>(&ImOutlineView::SetScrollOffsetProperty),
            static_cast<float& (ImOutlineView::*)()>(&ImOutlineView::GetScrollOffsetProperty),
            "Vertical scroll offset")
        .RegisterProperty(PropertyType::Struct, "Style", &ImOutlineView::Style_, "Outline view style");
    END_DECLARE_OBJECT()

public:
    using FSelectionChangedEvent = TMulticastDelegate<ImOutlineView&, ImOutlineItem*>;
    using FExpandedChangedEvent = TMulticastDelegate<ImOutlineView&, ImOutlineItem&, bool>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImOutlineView&, ImOutlineItem&, FVector2>;

    ImOutlineView();
    virtual ~ImOutlineView() = default;

    ImOutlineItem* AddRootItem(const std::shared_ptr<ImWidget>& content);
    ImOutlineItem* AddChildItem(ImOutlineItem* parent, const std::shared_ptr<ImWidget>& content);
    void RemoveItem(ImOutlineItem* item);
    void ClearItems();

    void SetSelectedItem(ImOutlineItem* item);
    ImOutlineItem* GetSelectedItem() const { return SelectedItem_; }
    void ClearSelection();

    bool ScrollToItem(ImOutlineItem* item, bool bCenterIfLarger = false);
    void ExpandAll();
    void CollapseAll();

    void SetStyle(const FOutlineViewStyle& style);
    const FOutlineViewStyle& GetStyle() const { return Style_; }

    void SetScrollOffset(float offset);
    float GetScrollOffset() const { return ScrollOffsetY_; }
    float GetMaxScrollOffset() const { return MaxScrollOffsetY_; }

    FSelectionChangedEvent OnSelectionChanged;
    FExpandedChangedEvent OnItemExpandedChanged;
    FContextMenuRequestedEvent OnItemContextMenuRequested;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnPreviewInputEvent(const FInputEvent& event) override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    struct FVisibleEntry {
        ImOutlineItem* Item = nullptr;
        int Depth = 0;
        float ContentY = 0.0f;
        float RowHeight = 0.0f;
        FGeometry RowGeometry;
        FGeometry IndicatorGeometry;
        FGeometry ContentGeometry;
    };

    void SetScrollOffsetProperty(float& offset) { SetScrollOffset(offset); }
    float& GetScrollOffsetProperty() { return ScrollOffsetY_; }

    void Relayout();
    void RebuildVisibleEntries(const FGeometry& viewportGeometry);
    void ClampScrollOffset();
    void UpdateVisibleEntryGeometries();
    void SetExpandedState(ImOutlineItem* item, bool expanded, bool bBroadcast);
    void SelectFallbackForCollapsedItem(ImOutlineItem* item);
    void SetSelectedItemInternal(ImOutlineItem* item, bool bBroadcast, bool bEnsureVisible);
    void EnsureAncestorsExpanded(ImOutlineItem* item, bool bBroadcast);
    void ExpandAllRecursive(ImOutlineItem& item, bool expanded);
    bool ContainsItem(const ImOutlineItem* item) const;
    bool IsDescendantOf(const ImOutlineItem* item, const ImOutlineItem* ancestor) const;
    ImOutlineItem* GetRootAncestor(ImOutlineItem* item) const;
    ImOutlineItem* GetParentItem(ImOutlineItem* item) const;
    FVisibleEntry* FindVisibleEntry(ImOutlineItem* item);
    const FVisibleEntry* FindVisibleEntry(ImOutlineItem* item) const;
    int FindVisibleIndex(ImOutlineItem* item) const;
    ImOutlineItem* ResolveItemAt(const FVector2& position);
    FVisibleEntry* ResolveEntryAt(const FVector2& position);
    bool RemoveItemRecursive(std::vector<std::unique_ptr<ImOutlineItem>>& items, ImOutlineItem* target);
    void RegisterContentWidget(const std::shared_ptr<ImWidget>& content);
    void RefreshRegisteredContentWidgets();
    void BeginScrollbarDrag(float grabOffset);
    void UpdateScrollbarDrag(const FVector2& cursorPosition);
    void EndScrollbarDrag();
    void HandleKeyboardNavigation(EKey key);
    void FlattenVisibleChildren(ImOutlineItem& item, int depth, float& cursorY);
    void CleanupInteractionStateForItemSubtree(ImOutlineItem& item);
    bool ContainsWidgetInItemSubtree(const std::shared_ptr<ImWidget>& widget, const ImOutlineItem& item) const;

    FOutlineViewStyle Style_;
    std::vector<std::unique_ptr<ImOutlineItem>> RootItems_;
    std::vector<FVisibleEntry> VisibleEntries_;
    ImOutlineItem* SelectedItem_ = nullptr;
    ImOutlineItem* HoveredItem_ = nullptr;
    FGeometry ViewportGeometry_;
    FGeometry VerticalScrollbarGeometry_;
    FGeometry VerticalThumbGeometry_;
    float ContentHeight_ = 0.0f;
    float ScrollOffsetY_ = 0.0f;
    float MaxScrollOffsetY_ = 0.0f;
    bool bLayoutDirty_ = true;
    bool bHoveredScrollbar_ = false;
    bool bDraggingScrollbar_ = false;
    float ActiveGrabOffset_ = 0.0f;
};

} // namespace ImWidgetV4
