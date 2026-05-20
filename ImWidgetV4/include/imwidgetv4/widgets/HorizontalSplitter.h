#pragma once

#include <imwidgetv4/widgets/PanelWidget.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

struct FHorizontalSplitterStyle : public ReflectableObject {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "FHorizontalSplitterStyle"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    float BarWidth = 4.0f;
    FColor Color = FColor::FromBytes(100, 100, 100, 255);
    FColor HoveredColor = FColor::FromBytes(120, 120, 120, 255);
    FColor ActiveColor = FColor::FromBytes(150, 150, 150, 255);
    float Rounding = 0.0f;
};

class ImHorizontalSplitterSlot : public ImPaddingSlot {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImHorizontalSplitterSlot"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImHorizontalSplitterSlot() = default;
    virtual ~ImHorizontalSplitterSlot() = default;

    void SetRatio(float ratio) { m_Ratio = ratio; }
    float GetRatio() const { return m_Ratio; }

    void SetMinSize(float minSize) { m_MinSize = minSize; }
    float GetMinSize() const { return m_MinSize; }

private:
    float m_Ratio = 1.0f;
    float m_MinSize = 30.0f;
};

class ImHorizontalSplitter : public ImPanelWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImHorizontalSplitter"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImHorizontalSplitter();
    virtual ~ImHorizontalSplitter() = default;

    virtual void AddChild(const Ptr& child) override;
    void AddPartWithSlot(const Ptr& child, std::unique_ptr<ImHorizontalSplitterSlot> slot = nullptr);
    ImHorizontalSplitterSlot* AddPart(
        const Ptr& child,
        float ratio = 1.0f,
        float minSize = 30.0f,
        const FMargin& padding = FMargin());

    void SetSplitterStyle(const FHorizontalSplitterStyle& style);
    const FHorizontalSplitterStyle& GetSplitterStyle() const { return GetEffectiveStyle(); }

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
    const FHorizontalSplitterStyle& GetEffectiveStyle() const;
    void ClearHoveredBar();
    void UpdateHoveredBar(const FVector2& mousePosition);
    int HitTestBarIndex(const FVector2& mousePosition) const;
    void BeginDrag(int barIndex, const FVector2& mousePosition);
    void UpdateDrag(const FVector2& mousePosition);
    void EndDrag();
    void RenderBars(const FPaintContext& paintContext) const;

    FHorizontalSplitterStyle m_Style;
    mutable FHorizontalSplitterStyle m_ResolvedThemeStyle;
    int m_HoveredBarIndex = -1;
    int m_DraggingBarIndex = -1;
    float m_DragStartMouseX = 0.0f;
    float m_DragStartLeftWidth = 0.0f;
    float m_DragStartRightWidth = 0.0f;
    float m_DragStartTotalRatio = 2.0f;
    std::vector<FGeometry> m_PartGeometries;
    std::vector<FGeometry> m_BarGeometries;
    bool m_bHasExplicitStyle = false;
};

} // namespace ImWidgetV4
