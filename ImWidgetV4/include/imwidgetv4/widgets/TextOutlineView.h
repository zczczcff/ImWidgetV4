#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/Image.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

enum class ETextOutlineDropZone : std::uint8_t {
    OnItem,
    BeforeItem,
    AfterItem
};

class ImTextOutlineItem : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImTextOutlineItem"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    std::string Text;
    FText TextValue;
    FImageBrush IconBrush;
    int IconType = -1;
    bool Expanded = false;

private:
    friend class ImTextOutlineView;

    ImTextOutlineItem* Parent = nullptr;
    std::vector<std::unique_ptr<ImTextOutlineItem>> Children;
};

struct FTextOutlineViewStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FTextOutlineViewStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

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
    float IconSize = 14.0f;
    float IconSpacing = 6.0f;
    float ScrollbarThickness = 10.0f;
    float ScrollbarPadding = 2.0f;
    FColor ScrollbarTrackColor = FColor::FromBytes(38, 45, 56);
    FColor ScrollbarThumbColor = FColor::FromBytes(88, 102, 119);
    FColor ScrollbarThumbHoveredColor = FColor::FromBytes(122, 143, 168);
    float ThumbMinLength = 28.0f;
    float WheelScrollStep = 28.0f;
};

class ImTextOutlineView : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImTextOutlineView"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FSelectionChangedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem*>;
    using FExpandedChangedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, bool>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, FVector2>;
    using FDeleteRequestedEvent = TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem*>;
    using FDragDetectedEvent =
        TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, std::shared_ptr<FDragDropOperation>&>;
    using FDropTestEvent =
        TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, ETextOutlineDropZone, const std::shared_ptr<FDragDropOperation>&, FVector2, bool&>;
    using FDropEvent =
        TMulticastDelegate<ImTextOutlineView&, ImTextOutlineItem&, ETextOutlineDropZone, const std::shared_ptr<FDragDropOperation>&, FVector2, bool&>;

    ImTextOutlineView();
    virtual ~ImTextOutlineView() = default;

    ImTextOutlineItem* AddRootItem(const std::string& text);
    ImTextOutlineItem* AddRootItem(const FText& text);
    ImTextOutlineItem* AddChildItem(ImTextOutlineItem* parent, const std::string& text);
    ImTextOutlineItem* AddChildItem(ImTextOutlineItem* parent, const FText& text);
    void RemoveItem(ImTextOutlineItem* item);
    void ClearItems();

    void SetSelectedItem(ImTextOutlineItem* item);
    ImTextOutlineItem* GetSelectedItem() const { return SelectedItem_; }
    void ClearSelection();

    bool ScrollToItem(ImTextOutlineItem* item, bool bCenterIfLarger = false);
    void ExpandAll();
    void CollapseAll();

    void SetStyle(const FTextOutlineViewStyle& style);
    const FTextOutlineViewStyle& GetStyle() const { return GetEffectiveStyle(); }

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
    const FTextOutlineViewStyle& GetEffectiveStyle() const;

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

    struct FDropTargetState {
        ImTextOutlineItem* Item = nullptr;
        ETextOutlineDropZone Zone = ETextOutlineDropZone::OnItem;

        bool operator==(const FDropTargetState& other) const
        {
            return Item == other.Item && Zone == other.Zone;
        }

        bool operator!=(const FDropTargetState& other) const
        {
            return !(*this == other);
        }
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
    std::string ResolveItemText(const ImTextOutlineItem& item) const;
    float MeasureTextWidth(const std::string& text) const;
    ETextOutlineDropZone ResolveDropZone(const FVisibleEntry& entry, const FVector2& position) const;
    FGeometry ResolveDropIndicatorGeometry(const FVisibleEntry& entry, ETextOutlineDropZone zone) const;

    FTextOutlineViewStyle Style_;
    mutable FTextOutlineViewStyle ResolvedThemeStyle_;
    std::vector<std::unique_ptr<ImTextOutlineItem>> RootItems_;
    std::vector<FVisibleEntry> VisibleEntries_;
    ImTextOutlineItem* SelectedItem_ = nullptr;
    ImTextOutlineItem* HoveredItem_ = nullptr;
    ImTextOutlineItem* PressedItem_ = nullptr;
    ImTextOutlineItem* DraggedItem_ = nullptr;
    FDropTargetState DropTarget_;
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
