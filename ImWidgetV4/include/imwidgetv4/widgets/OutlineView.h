#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

class ImOutlineItem : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImOutlineItem"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    bool Expanded = false;

private:
    friend class ImOutlineView;

    ImOutlineItem* Parent = nullptr;
    std::shared_ptr<ImWidget> ContentWidget;
    std::vector<std::unique_ptr<ImOutlineItem>> Children;
};

struct FOutlineViewStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FOutlineViewStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImOutlineView"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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

    void SetItemExpanded(ImOutlineItem* item, bool expanded, bool bBroadcast = true);
    bool IsItemExpanded(const ImOutlineItem* item) const;

    bool ScrollToItem(ImOutlineItem* item, bool bCenterIfLarger = false);
    void ExpandAll();
    void CollapseAll();

    void SetStyle(const FOutlineViewStyle& style);
    const FOutlineViewStyle& GetStyle() const { return GetEffectiveStyle(); }

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
    const FOutlineViewStyle& GetEffectiveStyle() const;

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
    mutable FOutlineViewStyle ResolvedThemeStyle_;
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
    bool bHasExplicitStyle_ = false;
    float ActiveGrabOffset_ = 0.0f;
};

} // namespace ImWidgetV4
