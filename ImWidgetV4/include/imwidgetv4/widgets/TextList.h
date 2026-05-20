#pragma once

#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Widget.h>
#include <string>
#include <vector>

namespace ImWidgetV4 {

struct FTextListStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FTextListStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor TextColor = FColor::FromBytes(238, 241, 245);
    FColor SelectionBackgroundColor = FColor::FromBytes(93, 149, 212, 176);
    FMargin Padding = FMargin(10.0f);
    FVector2 MinDesiredSize {320.0f, 220.0f};
    float CornerRadius = 7.0f;
    float BorderThickness = 1.0f;
    float FontSize = 16.0f;
    float LineSpacing = 1.15f;
    float ScrollbarThickness = 10.0f;
    float ScrollbarPadding = 2.0f;
    FColor ScrollbarTrackColor = FColor::FromBytes(38, 45, 56);
    FColor ScrollbarThumbColor = FColor::FromBytes(88, 102, 119);
    FColor ScrollbarThumbHoveredColor = FColor::FromBytes(122, 143, 168);
    float ThumbMinLength = 28.0f;
    float WheelScrollStep = 32.0f;
    float AutoScrollEdgePadding = 24.0f;
    float AutoScrollSpeed = 8.0f;
};

class ImTextList : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImTextList"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    struct FTextLine {
        std::string Text;
        int ItemIndex = -1;
        float ContentY = 0.0f;
        FVector2 Size {0.0f, 0.0f};
        std::vector<float> CharOffsets;
    };

    struct FItemLayout {
        std::size_t FirstLineIndex = 0;
        std::size_t LineCount = 0;
        float Top = 0.0f;
        float Bottom = 0.0f;
    };

    struct FSelectionPoint {
        int LineIndex = -1;
        int CharIndex = -1;

        bool IsValid() const { return LineIndex >= 0 && CharIndex >= 0; }
        bool operator==(const FSelectionPoint& other) const {
            return LineIndex == other.LineIndex && CharIndex == other.CharIndex;
        }
    };

    ImTextList();
    virtual ~ImTextList() = default;

    void SetItems(const std::vector<std::string>& items);
    void SetItems(const std::vector<FText>& items);
    const std::vector<std::string>& GetItems() const { return m_Items; }
    void AddItem(const std::string& item);
    void AddItem(const FText& item);
    void ClearItems();
    void ModifyItem(int index, const std::string& item);
    void ModifyItem(int index, const FText& item);
    void RemoveItem(int index);

    void SetStyle(const FTextListStyle& style);
    const FTextListStyle& GetStyle() const { return GetEffectiveStyle(); }

    void SetTextColor(const FColor& color);
    const FColor& GetTextColor() const { return GetEffectiveStyle().TextColor; }
    void SetItemColor(int index, const FColor& color);
    FColor GetItemColor(int index) const;
    void SetAllItemsColor(const FColor& color);

    void SetLineSpacing(float spacing);
    float GetLineSpacing() const { return GetEffectiveStyle().LineSpacing; }

    void SetScrollOffset(float offset);
    float GetScrollOffset() const { return m_ScrollOffsetY; }
    float GetMaxScrollOffset() const { return m_MaxScrollOffsetY; }
    bool ScrollToItem(int index, bool bCenterIfLarger = false);

    bool HasSelection() const;
    void ClearSelection();
    std::string GetSelectedText() const;
    void CopySelectionToClipboard() const;

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;

protected:
    virtual void OnFocusChanged(bool bHasFocus) override;

private:
    void SetItemsProperty(std::vector<std::string>& items) { SetItems(items); }
    std::vector<std::string>& GetItemsProperty() { return m_Items; }
    void SetScrollOffsetProperty(float& offset) { SetScrollOffset(offset); }
    float& GetScrollOffsetProperty() { return m_ScrollOffsetY; }

    void Relayout();
    void RebuildLayout(float wrapWidth);
    void ClampScrollOffset();
    void BeginScrollbarDrag(float grabOffset);
    void UpdateScrollbarDrag(const FVector2& cursorPosition);
    void EndScrollbarDrag();
    void BeginTextSelection(const FVector2& cursorPosition);
    void UpdateTextSelection(const FVector2& cursorPosition);
    void EndTextSelection();
    void ApplyAutoScrollForSelection(const FVector2& cursorPosition);
    FSelectionPoint ResolveSelectionPointAt(const FVector2& cursorPosition) const;
    void NormalizeSelection(FSelectionPoint& start, FSelectionPoint& end) const;
    bool IsNavigationShortcut(const FInputEvent& event) const;
    float ResolveWrappedLineHeight() const;
    float ResolveLineStride(float lineHeight) const;
    std::string ResolveItemText(int index) const;
    void SyncLocalizedItemsFromSerializableItems();
    const FTextListStyle& GetEffectiveStyle() const;

    FTextListStyle m_Style;
    mutable FTextListStyle m_ResolvedThemeStyle;
    std::vector<std::string> m_Items;
    std::vector<FText> m_ItemTexts;
    std::vector<FColor> m_ItemColors;
    std::vector<bool> m_HasExplicitItemColors;
    std::vector<FTextLine> m_Lines;
    std::vector<FItemLayout> m_ItemLayouts;
    FGeometry m_CachedViewportGeometry;
    FGeometry m_VerticalScrollbarGeometry;
    FGeometry m_VerticalThumbGeometry;
    float m_ContentHeight = 0.0f;
    float m_ScrollOffsetY = 0.0f;
    float m_MaxScrollOffsetY = 0.0f;
    bool m_bLayoutDirty = true;
    float m_LastLayoutWrapWidth = -1.0f;
    float m_LastLayoutHeight = -1.0f;
    bool m_bHoveredScrollbar = false;
    bool m_bDraggingScrollbar = false;
    bool m_bDraggingSelection = false;
    bool m_bHasExplicitStyle = false;
    float m_ActiveGrabOffset = 0.0f;
    FSelectionPoint m_SelectionAnchor;
    FSelectionPoint m_SelectionCursor;
};

} // namespace ImWidgetV4
