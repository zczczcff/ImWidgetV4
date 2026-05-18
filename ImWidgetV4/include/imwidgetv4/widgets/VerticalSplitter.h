#pragma once

#include <imwidgetv4/widgets/PanelWidget.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

struct FVerticalSplitterStyle : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(FVerticalSplitterStyle, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Float, "BarHeight", &FVerticalSplitterStyle::BarHeight, "Bar height")
        .RegisterProperty(PropertyType::Color, "Color", &FVerticalSplitterStyle::Color, "Bar color")
        .RegisterProperty(PropertyType::Color, "HoveredColor", &FVerticalSplitterStyle::HoveredColor, "Hovered bar color")
        .RegisterProperty(PropertyType::Color, "ActiveColor", &FVerticalSplitterStyle::ActiveColor, "Active bar color")
        .RegisterProperty(PropertyType::Float, "Rounding", &FVerticalSplitterStyle::Rounding, "Bar rounding");
    END_DECLARE_OBJECT()

public:
    float BarHeight = 4.0f;
    FColor Color = FColor::FromBytes(100, 100, 100, 255);
    FColor HoveredColor = FColor::FromBytes(120, 120, 120, 255);
    FColor ActiveColor = FColor::FromBytes(150, 150, 150, 255);
    float Rounding = 0.0f;
};

class ImVerticalSplitterSlot : public ImPaddingSlot {
    DECLARE_OBJECT_WITH_PARENT(ImVerticalSplitterSlot, ImPaddingSlot)
    registrar
        .RegisterProperty(PropertyType::Float, "Ratio", &ImVerticalSplitterSlot::m_Ratio, "Part ratio")
        .RegisterProperty(PropertyType::Float, "MinSize", &ImVerticalSplitterSlot::m_MinSize, "Minimum part size");
    END_DECLARE_OBJECT()

public:
    ImVerticalSplitterSlot() = default;
    virtual ~ImVerticalSplitterSlot() = default;

    void SetRatio(float ratio) { m_Ratio = ratio; }
    float GetRatio() const { return m_Ratio; }

    void SetMinSize(float minSize) { m_MinSize = minSize; }
    float GetMinSize() const { return m_MinSize; }

private:
    float m_Ratio = 1.0f;
    float m_MinSize = 30.0f;
};

class ImVerticalSplitter : public ImPanelWidget {
    DECLARE_OBJECT_WITH_PARENT(ImVerticalSplitter, ImPanelWidget)
    registrar
        .RegisterProperty(PropertyType::Struct, "Style", &ImVerticalSplitter::m_Style, "Vertical splitter style");
    END_DECLARE_OBJECT()

public:
    ImVerticalSplitter();
    virtual ~ImVerticalSplitter() = default;

    virtual void AddChild(const Ptr& child) override;
    void AddPartWithSlot(const Ptr& child, std::unique_ptr<ImVerticalSplitterSlot> slot = nullptr);
    ImVerticalSplitterSlot* AddPart(
        const Ptr& child,
        float ratio = 1.0f,
        float minSize = 30.0f,
        const FMargin& padding = FMargin());

    void SetSplitterStyle(const FVerticalSplitterStyle& style);
    const FVerticalSplitterStyle& GetSplitterStyle() const { return GetEffectiveStyle(); }

    void SetPartMinSize(int index, float minSize);

    virtual std::unique_ptr<ImSlot> CreateSlot() override;
    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual FReply OnInputEvent(const FInputEvent& event) override;
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;
    virtual void Relayout();

protected:
    int GetHoveredBarIndex() const { return m_HoveredBarIndex; }
    int GetDraggingBarIndex() const { return m_DraggingBarIndex; }
    const std::vector<FGeometry>& GetPartGeometries() const { return m_PartGeometries; }
    const std::vector<FGeometry>& GetBarGeometries() const { return m_BarGeometries; }

private:
    const FVerticalSplitterStyle& GetEffectiveStyle() const;
    void ClearHoveredBar();
    void UpdateHoveredBar(const FVector2& mousePosition);
    int HitTestBarIndex(const FVector2& mousePosition) const;
    void BeginDrag(int barIndex, const FVector2& mousePosition);
    void UpdateDrag(const FVector2& mousePosition);
    void EndDrag();
    void RenderBars(const FPaintContext& paintContext) const;

    FVerticalSplitterStyle m_Style;
    mutable FVerticalSplitterStyle m_ResolvedThemeStyle;
    int m_HoveredBarIndex = -1;
    int m_DraggingBarIndex = -1;
    float m_DragStartMouseY = 0.0f;
    float m_DragStartTopHeight = 0.0f;
    float m_DragStartBottomHeight = 0.0f;
    float m_DragStartTotalRatio = 2.0f;
    std::vector<FGeometry> m_PartGeometries;
    std::vector<FGeometry> m_BarGeometries;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
