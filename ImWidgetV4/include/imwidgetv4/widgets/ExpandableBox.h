#pragma once

#include <imwidgetv4/core/Delegate.h>
#include <imwidgetv4/widgets/PanelWidget.h>
#include <memory>

namespace ImWidgetV4 {

struct FExpandableBoxStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FExpandableBoxStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Color, "HeaderBackgroundColor", &FExpandableBoxStyle::HeaderBackgroundColor, "Header background color")
        .RegisterProperty(PropertyType::Color, "HeaderHoveredBackgroundColor", &FExpandableBoxStyle::HeaderHoveredBackgroundColor, "Header hovered background color")
        .RegisterProperty(PropertyType::Color, "HeaderPressedBackgroundColor", &FExpandableBoxStyle::HeaderPressedBackgroundColor, "Header pressed background color")
        .RegisterProperty(PropertyType::Color, "BodyBackgroundColor", &FExpandableBoxStyle::BodyBackgroundColor, "Body background color")
        .RegisterProperty(PropertyType::Color, "BorderColor", &FExpandableBoxStyle::BorderColor, "Border color")
        .RegisterProperty(PropertyType::Color, "FocusedOutlineColor", &FExpandableBoxStyle::FocusedOutlineColor, "Focused outline color")
        .RegisterProperty(PropertyType::Color, "IndicatorColor", &FExpandableBoxStyle::IndicatorColor, "Indicator color")
        .RegisterProperty(PropertyType::Color, "IndicatorHoveredColor", &FExpandableBoxStyle::IndicatorHoveredColor, "Indicator hovered color")
        .RegisterProperty(PropertyType::Struct, "HeaderPadding", &FExpandableBoxStyle::HeaderPadding, "Header padding")
        .RegisterProperty(PropertyType::Struct, "BodyPadding", &FExpandableBoxStyle::BodyPadding, "Body padding")
        .RegisterProperty(PropertyType::Float, "IndicatorSize", &FExpandableBoxStyle::IndicatorSize, "Indicator size")
        .RegisterProperty(PropertyType::Float, "IndicatorSpacing", &FExpandableBoxStyle::IndicatorSpacing, "Indicator spacing")
        .RegisterProperty(PropertyType::Float, "BorderThickness", &FExpandableBoxStyle::BorderThickness, "Border thickness")
        .RegisterProperty(PropertyType::Float, "CornerRadius", &FExpandableBoxStyle::CornerRadius, "Corner radius")
        .RegisterProperty(PropertyType::Vec2, "MinDesiredSize", &FExpandableBoxStyle::MinDesiredSize, "Minimum desired size");
    END_DECLARE_OBJECT()

public:
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
    DECLARE_OBJECT_WITH_PARENT(ImExpandableBox, ImPanelWidget)
    registrar
        .RegisterProperty(
            PropertyType::Bool,
            "Expanded",
            static_cast<void (ImExpandableBox::*)(bool&)>(&ImExpandableBox::SetExpandedProperty),
            static_cast<bool& (ImExpandableBox::*)()>(&ImExpandableBox::GetExpandedProperty),
            "Whether the expandable box is expanded")
        .RegisterProperty(PropertyType::Struct, "Style", &ImExpandableBox::m_Style, "Expandable box style");
    END_DECLARE_OBJECT()

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
    const FExpandableBoxStyle& GetStyle() const { return GetEffectiveStyle(); }

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
    void SetExpandedProperty(bool& bExpanded) { SetExpanded(bExpanded); }
    bool& GetExpandedProperty() { return m_bExpanded; }

    void RefreshVisibleChildren();
    void ClearBodyInteractionState();
    bool IsDescendantOfBody(const std::shared_ptr<ImWidget>& widget) const;
    void SetHovered(bool bHovered);
    void SetPressed(bool bPressed);
    const FExpandableBoxStyle& GetEffectiveStyle() const;
    bool ContainsIndicatorHotspot(const FVector2& position) const;
    float ComputeHeaderHeight() const;
    float ComputeBodyHeight() const;
    float ComputeVisibleHeight() const;

    FExpandableBoxStyle m_Style;
    mutable FExpandableBoxStyle m_ResolvedThemeStyle;
    std::shared_ptr<ImWidget> m_HeaderWidget;
    std::shared_ptr<ImWidget> m_BodyWidget;
    FGeometry m_HeaderGeometry;
    FGeometry m_BodyGeometry;
    FGeometry m_IndicatorHotspotGeometry;
    bool m_bExpanded = false;
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
