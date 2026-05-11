#pragma once

#include <imwidgetv4/core/Localization.h>
#include <imwidgetv4/core/Widget.h>
#include <string>
#include <vector>

namespace ImWidgetV4 {

struct FTextListStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FTextListStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "BackgroundColor", &FTextListStyle::BackgroundColor, "Background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FTextListStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FTextListStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "TextColor", &FTextListStyle::TextColor, "Text color")
        .RegisterProperty(PropertyType::Color, "SelectionBackgroundColor", &FTextListStyle::SelectionBackgroundColor, "Selection background color")
        .RegisterProperty(PropertyType::Struct, "Padding", &FTextListStyle::Padding, "Content padding")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FTextListStyle::MinDesiredSize, "Minimum desired size")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FTextListStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FTextListStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "FontSize", &FTextListStyle::FontSize, "Font size")
        .RegisterProperty(PropertyType::Float, "LineSpacing", &FTextListStyle::LineSpacing, "Line spacing")
        .RegisterProperty(PropertyType::Float, "ScrollbarThickness", &FTextListStyle::ScrollbarThickness, "Scrollbar thickness")
        .RegisterProperty(PropertyType::Float, "ScrollbarPadding", &FTextListStyle::ScrollbarPadding, "Scrollbar padding")
        .RegisterProperty(PropertyType::Color, "ScrollbarTrackColor", &FTextListStyle::ScrollbarTrackColor, "Scrollbar track color")
        .RegisterProperty(PropertyType::Color, "ScrollbarThumbColor", &FTextListStyle::ScrollbarThumbColor, "Scrollbar thumb color")
        .RegisterProperty(PropertyType::Color, "ScrollbarThumbHoveredColor", &FTextListStyle::ScrollbarThumbHoveredColor, "Hovered scrollbar thumb color")
        .RegisterProperty(PropertyType::Float, "ThumbMinLength", &FTextListStyle::ThumbMinLength, "Minimum thumb length")
        .RegisterProperty(PropertyType::Float, "WheelScrollStep", &FTextListStyle::WheelScrollStep, "Wheel scroll step")
        .RegisterProperty(PropertyType::Float, "AutoScrollEdgePadding", &FTextListStyle::AutoScrollEdgePadding, "Auto-scroll edge padding")
        .RegisterProperty(PropertyType::Float, "AutoScrollSpeed", &FTextListStyle::AutoScrollSpeed, "Auto-scroll speed");
    END_DECLARE_OBJECT()

public:
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
    DECLARE_OBJECT_WITH_PARENT(ImTextList, ImWidget)
    registrar
        .RegisterProperty(
            PropertyType::StringArray,
            "Items",
            static_cast<void (ImTextList::*)(std::vector<std::string>&)>(&ImTextList::SetItemsProperty),
            static_cast<std::vector<std::string>& (ImTextList::*)()>(&ImTextList::GetItemsProperty),
            "List items")
        .RegisterProperty(
            PropertyType::Float,
            "ScrollOffset",
            static_cast<void (ImTextList::*)(float&)>(&ImTextList::SetScrollOffsetProperty),
            static_cast<float& (ImTextList::*)()>(&ImTextList::GetScrollOffsetProperty),
            "Vertical scroll offset")
        .RegisterProperty(PropertyType::Struct, "Style", &ImTextList::m_Style, "Text list style");
    END_DECLARE_OBJECT()

public:
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
    const FTextListStyle& GetStyle() const { return m_Style; }

    void SetTextColor(const FColor& color);
    const FColor& GetTextColor() const { return m_Style.TextColor; }
    void SetItemColor(int index, const FColor& color);
    FColor GetItemColor(int index) const;
    void SetAllItemsColor(const FColor& color);

    void SetLineSpacing(float spacing);
    float GetLineSpacing() const { return m_Style.LineSpacing; }

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

    FTextListStyle m_Style;
    std::vector<std::string> m_Items;
    std::vector<FText> m_ItemTexts;
    std::vector<FColor> m_ItemColors;
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
    float m_ActiveGrabOffset = 0.0f;
    FSelectionPoint m_SelectionAnchor;
    FSelectionPoint m_SelectionCursor;
};

} // namespace ImWidgetV4
