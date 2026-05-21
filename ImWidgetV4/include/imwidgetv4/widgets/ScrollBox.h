#pragma once

#include <imwidgetv4/widgets/PanelWidget.h>
#include <memory>

namespace ImWidgetV4 {

struct FScrollBoxStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FScrollBoxStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    FColor BackgroundColor = FColor::FromBytes(24, 28, 34);
    FColor BorderColor = FColor::FromBytes(16, 19, 23);
    float BorderThickness = 1.0f;
    float CornerRadius = 6.0f;
    FMargin Padding = FMargin(8.0f);
    float ScrollbarThickness = 10.0f;
    float ScrollbarPadding = 2.0f;
    FColor ScrollbarTrackColor = FColor::FromBytes(38, 45, 56);
    FColor ScrollbarThumbColor = FColor::FromBytes(88, 102, 119);
    FColor ScrollbarThumbHoveredColor = FColor::FromBytes(122, 143, 168);
    float ThumbMinLength = 28.0f;
    float WheelScrollStep = 32.0f;
};

class ImScrollBox : public ImPanelWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImScrollBox"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImScrollBox();
    virtual ~ImScrollBox() = default;

    void SetContent(const std::shared_ptr<ImWidget>& child);
    std::shared_ptr<ImWidget> GetContent() const { return m_Content; }

    virtual void AddChild(const Ptr& child) override;
    virtual bool RemoveChild(const Ptr& child) override;
    virtual void ClearChildren() override;

    void SetScrollOffset(const FVector2& scrollOffset);
    const FVector2& GetScrollOffset() const { return m_ScrollOffset; }
    void ScrollBy(const FVector2& delta);
    FVector2 GetMaxScrollOffset() const { return m_MaxScrollOffset; }
    void ScrollToStart();
    void ScrollToEnd();
    bool ScrollToWidget(const std::shared_ptr<ImWidget>& widget, bool bCenterIfLarger = false);

    void SetStyle(const FScrollBoxStyle& style);
    const FScrollBoxStyle& GetStyle() const { return GetEffectiveStyle(); }

    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    virtual void PostDeserializeFromJson() override;

    void Relayout();

private:
    const FScrollBoxStyle& GetEffectiveStyle() const;
    void SetScrollOffsetProperty(FVector2& scrollOffset) { SetScrollOffset(scrollOffset); }
    FVector2& GetScrollOffsetProperty() { return m_ScrollOffset; }

    enum class EHoveredScrollbar {
        None,
        Horizontal,
        Vertical
    };

    enum class EActiveScrollbar {
        None,
        Horizontal,
        Vertical
    };

    void ClampScrollOffset();
    void UpdateHoveredScrollbar(const FVector2& cursorPosition);
    void BeginScrollbarDrag(EActiveScrollbar activeScrollbar, float grabOffset);
    void UpdateScrollbarDrag(const FVector2& cursorPosition);
    void EndScrollbarDrag();
    bool IsDescendantOfContent(const std::shared_ptr<ImWidget>& widget) const;

    FScrollBoxStyle m_Style;
    mutable FScrollBoxStyle m_ResolvedThemeStyle;
    std::shared_ptr<ImWidget> m_Content;
    FVector2 m_ScrollOffset {0.0f, 0.0f};
    FVector2 m_MaxScrollOffset {0.0f, 0.0f};
    FVector2 m_CachedContentSize {0.0f, 0.0f};
    FGeometry m_CachedViewportGeometry;
    FGeometry m_HorizontalScrollbarGeometry;
    FGeometry m_VerticalScrollbarGeometry;
    FGeometry m_HorizontalThumbGeometry;
    FGeometry m_VerticalThumbGeometry;
    bool m_bShowHorizontalScrollbar = false;
    bool m_bShowVerticalScrollbar = false;
    EHoveredScrollbar m_HoveredScrollbar = EHoveredScrollbar::None;
    EActiveScrollbar m_ActiveScrollbar = EActiveScrollbar::None;
    float m_ActiveGrabOffset = 0.0f;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
