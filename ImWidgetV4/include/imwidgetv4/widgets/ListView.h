#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Widget.h>
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ImWidgetV4 {

struct FListViewStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FListViewStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor HoveredRowColor = FColor::FromBytes(44, 52, 63);
    FColor SelectedRowColor = FColor::FromBytes(74, 112, 168);
    FColor SelectedFocusedRowColor = FColor::FromBytes(92, 141, 214);
    FMargin Padding = FMargin(8.0f);
    FMargin RowPadding = FMargin(8.0f, 8.0f, 4.0f, 4.0f);
    FVector2 MinDesiredSize {280.0f, 220.0f};
    float CornerRadius = 6.0f;
    float BorderThickness = 1.0f;
    float RowMinHeight = 28.0f;
    float ScrollbarThickness = 10.0f;
    float ScrollbarPadding = 2.0f;
    FColor ScrollbarTrackColor = FColor::FromBytes(38, 45, 56);
    FColor ScrollbarThumbColor = FColor::FromBytes(88, 102, 119);
    FColor ScrollbarThumbHoveredColor = FColor::FromBytes(122, 143, 168);
    float ThumbMinLength = 28.0f;
    float WheelScrollStep = 28.0f;
};

class ImListView : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImListView"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    using FOnGenerateRow = std::function<std::shared_ptr<ImWidget>(std::size_t index)>;
    using FSelectionChangedEvent = TMulticastDelegate<ImListView&, int>;
    using FContextMenuRequestedEvent = TMulticastDelegate<ImListView&, int, FVector2>;

    ImListView();
    virtual ~ImListView() = default;

    void SetItemCount(std::size_t count);
    std::size_t GetItemCount() const { return ItemCount_; }

    void SetOnGenerateRow(FOnGenerateRow callback);
    void RequestRefresh();

    void SetSelectedIndex(int index);
    int GetSelectedIndex() const { return SelectedIndex_; }
    bool HasSelection() const;
    void ClearSelection();

    bool ScrollToIndex(int index, bool bCenterIfLarger = false);
    void SetScrollOffset(float offset);
    float GetScrollOffset() const { return ScrollOffsetY_; }
    float GetMaxScrollOffset() const { return MaxScrollOffsetY_; }

    void SetEmptyContent(const std::shared_ptr<ImWidget>& widget);
    std::shared_ptr<ImWidget> GetEmptyContent() const;
    void SetDefaultEmptyText(const FText& text);
    const FText& GetDefaultEmptyText() const { return DefaultEmptyText_; }

    void SetStyle(const FListViewStyle& style);
    const FListViewStyle& GetStyle() const { return GetEffectiveStyle(); }

    FSelectionChangedEvent OnSelectionChanged;
    FContextMenuRequestedEvent OnItemContextMenuRequested;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnPreviewInputEvent(const FInputEvent& event) override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    const FListViewStyle& GetEffectiveStyle() const;

    struct FVisibleEntry {
        int Index = -1;
        float ContentY = 0.0f;
        float RowHeight = 0.0f;
        FGeometry RowGeometry;
        FGeometry ContentGeometry;
        std::shared_ptr<ImWidget> RowWidget;
    };

    static constexpr int OverscanItemCount_ = 2;

    void SetItemCountProperty(int& count);
    int& GetItemCountProperty();
    void SetSelectedIndexProperty(int& index);
    int& GetSelectedIndexProperty();
    void SetScrollOffsetProperty(float& offset);
    float& GetScrollOffsetProperty();

    void Relayout();
    void RebuildVisibleEntries(const FGeometry& viewportGeometry, bool bMeasureHeights);
    void UpdateVisibleEntryGeometries();
    void ClampScrollOffset();
    void RefreshRegisteredChildren();
    void EnsureDefaultEmptyContent();
    void UpdateEmptyContentGeometry();
    void SetSelectedIndexInternal(int index, bool bBroadcast, bool bEnsureVisible);
    void HandleKeyboardNavigation(EKey key);
    void BeginScrollbarDrag(float grabOffset);
    void UpdateScrollbarDrag(const FVector2& cursorPosition);
    void EndScrollbarDrag();
    FVisibleEntry* ResolveEntryAt(const FVector2& position);
    const FVisibleEntry* ResolveEntryAt(const FVector2& position) const;
    float EstimateRowHeight() const;
    float GetRowHeightForIndex(int index) const;
    float GetRowTop(int index) const;
    bool IsValidIndex(int index) const;
    std::shared_ptr<ImWidget> EnsureRealizedRow(int index);
    void ReleaseRealizedRow(int index);
    void ReleaseAllRealizedRows();
    void CleanupInteractionStateForWidgetSubtree(const std::shared_ptr<ImWidget>& widget);
    bool ContainsWidgetInSubtree(const std::shared_ptr<ImWidget>& widget, const std::shared_ptr<ImWidget>& subtreeRoot) const;

    FListViewStyle Style_;
    mutable FListViewStyle ResolvedThemeStyle_;
    FOnGenerateRow OnGenerateRow_;
    std::shared_ptr<ImWidget> EmptyContent_;
    std::shared_ptr<ImWidget> DefaultEmptyContent_;
    FText DefaultEmptyText_ = FText::FromString("No items");
    std::vector<float> RowHeightCache_;
    std::unordered_map<int, std::shared_ptr<ImWidget>> RealizedRows_;
    std::vector<FVisibleEntry> VisibleEntries_;
    FGeometry ViewportGeometry_;
    FGeometry VerticalScrollbarGeometry_;
    FGeometry VerticalThumbGeometry_;
    float ContentHeight_ = 0.0f;
    float ScrollOffsetY_ = 0.0f;
    float MaxScrollOffsetY_ = 0.0f;
    std::size_t ItemCount_ = 0;
    int SelectedIndex_ = -1;
    int HoveredRowIndex_ = -1;
    bool bLayoutDirty_ = true;
    bool bHoveredScrollbar_ = false;
    bool bDraggingScrollbar_ = false;
    bool bHasExplicitStyle_ = false;
    float ActiveGrabOffset_ = 0.0f;
    int ReflectedItemCount_ = 0;
};

} // namespace ImWidgetV4
