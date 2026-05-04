#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <memory>

namespace ImWidgetV4 {

struct FExpandableBoxStyle {
    FColor HeaderBackgroundColor = FColor::FromBytes(36, 43, 53);
    FColor HeaderHoveredBackgroundColor = FColor::FromBytes(47, 56, 68);
    FColor HeaderPressedBackgroundColor = FColor::FromBytes(28, 34, 42);
    FColor BodyBackgroundColor = FColor::FromBytes(24, 29, 37);
    FColor BorderColor = FColor::FromBytes(12, 16, 21);
    FColor FocusedOutlineColor = FColor::FromBytes(103, 177, 255);
    FColor IndicatorColor = FColor::FromBytes(232, 237, 243);
    FColor IndicatorHoveredColor = FColor::FromBytes(255, 214, 102);
    FMargin HeaderPadding = FMargin(12.0f, 12.0f, 8.0f, 8.0f);
    FMargin BodyPadding = FMargin(12.0f, 12.0f, 10.0f, 10.0f);
    float IndicatorSize = 10.0f;
    float IndicatorSpacing = 8.0f;
    float BorderThickness = 1.0f;
    float CornerRadius = 8.0f;
    FVector2 MinDesiredSize {180.0f, 36.0f};
};

class ImExpandableBox : public ImPanelWidget {
public:
    using FExpandedStateChangedEvent = TMulticastDelegate<ImExpandableBox&, bool>;
    using FExpandableBoxEvent = TMulticastDelegate<ImExpandableBox&>;

    ImExpandableBox();
    virtual ~ImExpandableBox() = default;

    void SetHeader(const Ptr& header);
    std::shared_ptr<ImWidget> GetHeader() const { return m_HeaderWidget; }

    void SetBody(const Ptr& body);
    std::shared_ptr<ImWidget> GetBody() const { return m_BodyWidget; }

    void SetExpanded(bool bExpanded);
    void ToggleExpanded();
    bool IsExpanded() const { return m_bExpanded; }

    void SetStyle(const FExpandableBoxStyle& style);
    const FExpandableBoxStyle& GetStyle() const { return m_Style; }

    bool IsHovered() const { return m_bHovered; }

    FExpandedStateChangedEvent OnExpandedStateChanged;
    FExpandableBoxEvent OnHoverBegin;
    FExpandableBoxEvent OnHoverEnd;

    virtual std::unique_ptr<ImSlot> CreateSlot() override;
    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    virtual void Relayout();

private:
    void RefreshVisibleChildren();
    void ClearBodyInteractionState();
    bool IsDescendantOfBody(const std::shared_ptr<ImWidget>& widget) const;
    void SetHovered(bool bHovered);
    void SetPressed(bool bPressed);
    bool ContainsIndicatorHotspot(const FVector2& position) const;
    float ComputeHeaderHeight() const;
    float ComputeBodyHeight() const;
    float ComputeVisibleHeight() const;

    FExpandableBoxStyle m_Style;
    std::shared_ptr<ImWidget> m_HeaderWidget;
    std::shared_ptr<ImWidget> m_BodyWidget;
    FGeometry m_HeaderGeometry;
    FGeometry m_BodyGeometry;
    FGeometry m_IndicatorHotspotGeometry;
    bool m_bExpanded = false;
    bool m_bHovered = false;
    bool m_bPressed = false;
};

} // namespace ImWidgetV4
