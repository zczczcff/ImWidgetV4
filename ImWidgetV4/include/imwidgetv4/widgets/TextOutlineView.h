#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

class ImTextOutlineItem : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(ImTextOutlineItem, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::String, "Text", &ImTextOutlineItem::Text, "Outline item text")
        .RegisterProperty(PropertyType::Bool, "Expanded", &ImTextOutlineItem::Expanded, "Whether the item is expanded");
    END_DECLARE_OBJECT()

public:
    std::string Text;
    bool Expanded = false;

private:
    friend class ImTextOutlineView;

    ImTextOutlineItem* Parent = nullptr;
    std::vector<std::unique_ptr<ImTextOutlineItem>> Children;
};

struct FTextOutlineViewStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FTextOutlineViewStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FTextOutlineViewStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FTextOutlineViewStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FTextOutlineViewStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "TextColor", &FTextOutlineViewStyle::TextColor, "Text color")
        .RegisterProperty(PropertyType::Color, "HoveredRowColor", &FTextOutlineViewStyle::HoveredRowColor, "Hovered row color")
        .RegisterProperty(PropertyType::Color, "SelectedRowColor", &FTextOutlineViewStyle::SelectedRowColor, "Selected row color")
        .RegisterProperty(PropertyType::Color, "SelectedFocusedRowColor", &FTextOutlineViewStyle::SelectedFocusedRowColor, "Selected focused row color")
        .RegisterProperty(PropertyType::Color, "IndicatorColor", &FTextOutlineViewStyle::IndicatorColor, "Expand indicator color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FTextOutlineViewStyle::Padding, "Outer padding")
        .RegisterProperty(PropertyType::Struct, "RowPadding", &FTextOutlineViewStyle::RowPadding, "Row padding")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FTextOutlineViewStyle::MinDesiredSize, "Minimum desired size")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FTextOutlineViewStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FTextOutlineViewStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "FontSize", &FTextOutlineViewStyle::FontSize, "Font size")
        .RegisterProperty(PropertyType::Float, "RowHeight", &FTextOutlineViewStyle::RowHeight, "Row height")
        .RegisterProperty(PropertyType::Float, "IndentWidth", &FTextOutlineViewStyle::IndentWidth, "Indent width")
        .RegisterProperty(PropertyType::Float, "IndicatorSize", &FTextOutlineViewStyle::IndicatorSize, "Indicator size")
        .RegisterProperty(PropertyType::Float, "IndicatorSpacing", &FTextOutlineViewStyle::IndicatorSpacing, "Indicator spacing")
        .RegisterProperty(PropertyType::Float, "ScrollbarThickness", &FTextOutlineViewStyle::ScrollbarThickness, "Scrollbar thickness")
        .RegisterProperty(PropertyType::Float, "ScrollbarPadding", &FTextOutlineViewStyle::ScrollbarPadding, "Scrollbar padding")
        .RegisterProperty(PropertyType::Color, "ScrollbarTrackColor", &FTextOutlineViewStyle::ScrollbarTrackColor, "Scrollbar track color")
        .RegisterProperty(PropertyType::Color, "ScrollbarThumbColor", &FTextOutlineViewStyle::ScrollbarThumbColor, "Scrollbar thumb color")
        .RegisterProperty(PropertyType::Color, "ScrollbarThumbHoveredColor", &FTextOutlineViewStyle::ScrollbarThumbHoveredColor, "Hovered scrollbar thumb color")
        .RegisterProperty(PropertyType::Float, "ThumbMinLength", &FTextOutlineViewStyle::ThumbMinLength, "Minimum thumb length")
        .RegisterProperty(PropertyType::Float, "WheelScrollStep", &FTextOutlineViewStyle::WheelScrollStep, "Wheel scroll step");
    END_DECLARE_OBJECT()

public:
    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor TextColor = FColor::FromBytes(238, 241, 245);
    FColor HoveredRowColor = FColor::FromBytes(44, 52, 63);
    FColor SelectedRowColor = FColor::FromBytes(74, 112, 168);
    FColor SelectedFocusedRowColor = FColor::FromBytes(92, 141, 214);
    FColor IndicatorColor = FColor::FromBytes(228, 232, 238);
    FMargin Padding = FMargin(8.0f);
    FMargin RowPadding = FMargin(6.0f, 8.0f, 4.0f, 4.0f);
    FVector2 MinDesiredSize {240.0f, 220.0f};
    float CornerRadius = 6.0f;
    float BorderThickness = 1.0f;
    float FontSize = 16.0f;
    float RowHeight = 26.0f;
    float IndentWidth = 18.0f;
    float IndicatorSize = 10.0f;
    float IndicatorSpacing = 6.0f;
    float ScrollbarThickness = 10.0f;
    float ScrollbarPadding = 2.0f;
    FColor ScrollbarTrackColor = FColor::FromBytes(38, 45, 56);
    FColor ScrollbarThumbColor = FColor::FromBytes(88, 102, 119);
    FColor ScrollbarThumbHoveredColor = FColor::FromBytes(122, 143, 168);
    float ThumbMinLength = 28.0f;
    float WheelScrollStep = 28.0f;
};

class ImTextOutlineView : public ImWidget {
    DECLARE_OBJECT_WITH_PARENT(ImTextOutlineView, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::Float,
            "ScrollOffset",
            static_cast<void (ImTextOutlineView::*)(float&)>(&ImTextOutlineView::SetScrollOffsetProperty),
            static_cast<float& (ImTextOutlineView::*)()>(&ImTextOutlineView::GetScrollOffsetProperty),
            "Vertical scroll offset")
        .RegisterProperty(PropertyType::Struct, "Style", &ImTextOutlineView::Style_, "Outline view style");
    END_DECLARE_OBJECT()

public:
    using FSelectionChangedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem*>;
    using FExpandedChangedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, bool>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, FVector2>;
    using FDeleteRequestedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem*>;
    using FDragDetectedEvent =
        TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, std::shared_ptr<FDragDropOperation>&>;
    using FDropTestEvent =
        TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, const std::shared_ptr<FDragDropOperation>&, FVector2, bool&>;
    using FDropEvent =
        TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, const std::shared_ptr<FDragDropOperation>&, FVector2, bool&>;

    ImTextOutlineView();
    virtual ~ImTextOutlineView() = default;

    ImTextOutlineItem* AddRootItem(const std::string& text);
    ImTextOutlineItem* AddChildItem(ImTextOutlineItem* parent, const std::string& text);
    void RemoveItem(ImTextOutlineItem* item);
    void ClearItems();

    void SetSelectedItem(ImTextOutlineItem* item);
    ImTextOutlineItem* GetSelectedItem() const { return SelectedItem_; }
    void ClearSelection();

    bool ScrollToItem(ImTextOutlineItem* item, bool bCenterIfLarger = false);
    void ExpandAll();
    void CollapseAll();

    void SetStyle(const FTextOutlineViewStyle& style);
    const FTextOutlineViewStyle& GetStyle() const { return Style_; }

    void SetScrollOffset(float offset);
    float GetScrollOffset() const { return ScrollOffsetY_; }
    float GetMaxScrollOffset() const { return MaxScrollOffsetY_; }

    FSelectionChangedEvent OnSelectionChanged;
    FExpandedChangedEvent OnItemExpandedChanged;
    FContextMenuRequestedEvent OnItemContextMenuRequested;
    FDeleteRequestedEvent OnDeleteRequested;
    FDragDetectedEvent OnItemDragDetected;
    FDropTestEvent OnItemDropTest;
    FDropEvent OnItemDropped;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual std::shared_ptr<FDragDropOperation> OnDragDetected(const FDragDetectEvent& event) override;
    virtual FReply OnDragEvent(const FDragDropEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    struct FVisibleEntry {
        ImTextOutlineItem* Item = nullptr;
        int Depth = 0;
        float TextWidth = 0.0f;
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
    void SetExpandedState(ImTextOutlineItem* item, bool expanded, bool bBroadcast);
    void SelectFallbackForCollapsedItem(ImTextOutlineItem* item);
    void SetSelectedItemInternal(ImTextOutlineItem* item, bool bBroadcast, bool bEnsureVisible);
    void EnsureAncestorsExpanded(ImTextOutlineItem* item, bool bBroadcast);
    void ExpandAllRecursive(ImTextOutlineItem& item, bool expanded);
    bool ContainsItem(const ImTextOutlineItem* item) const;
    bool IsDescendantOf(const ImTextOutlineItem* item, const ImTextOutlineItem* ancestor) const;
    ImTextOutlineItem* GetRootAncestor(ImTextOutlineItem* item) const;
    ImTextOutlineItem* GetParentItem(ImTextOutlineItem* item) const;
    FVisibleEntry* FindVisibleEntry(ImTextOutlineItem* item);
    const FVisibleEntry* FindVisibleEntry(ImTextOutlineItem* item) const;
    int FindVisibleIndex(ImTextOutlineItem* item) const;
    ImTextOutlineItem* ResolveItemAt(const FVector2& position);
    FVisibleEntry* ResolveEntryAt(const FVector2& position);
    bool RemoveItemRecursive(std::vector<std::unique_ptr<ImTextOutlineItem>>& items, ImTextOutlineItem* target);
    void BeginScrollbarDrag(float grabOffset);
    void UpdateScrollbarDrag(const FVector2& cursorPosition);
    void EndScrollbarDrag();
    void HandleKeyboardNavigation(EKey key);
    void FlattenVisibleChildren(ImTextOutlineItem& item, int depth, float& cursorY);
    float MeasureTextWidth(const std::string& text) const;

    FTextOutlineViewStyle Style_;
    std::vector<std::unique_ptr<ImTextOutlineItem>> RootItems_;
    std::vector<FVisibleEntry> VisibleEntries_;
    ImTextOutlineItem* SelectedItem_ = nullptr;
    ImTextOutlineItem* HoveredItem_ = nullptr;
    ImTextOutlineItem* PressedItem_ = nullptr;
    ImTextOutlineItem* DraggedItem_ = nullptr;
    ImTextOutlineItem* DropTargetItem_ = nullptr;
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
