#pragma once

#include <imwidgetv4/widgets/PanelWidget.h>

namespace ImWidgetV4 {

class ImCanvasPanelSlot : public ImSlot {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImCanvasPanelSlot"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImCanvasPanelSlot() = default;
    virtual ~ImCanvasPanelSlot() = default;

    void SetRelativePosition(const FVector2& relativePosition) { m_RelativePosition = relativePosition; }
    const FVector2& GetRelativePosition() const { return m_RelativePosition; }

    void SetRelativeSize(const FVector2& relativeSize) { m_RelativeSize = relativeSize; }
    const FVector2& GetRelativeSize() const { return m_RelativeSize; }

    void SetAutoSize(bool bAutoSize) { m_bAutoSize = bAutoSize; }
    bool GetAutoSize() const { return m_bAutoSize; }

private:
    FVector2 m_RelativePosition {0.0f, 0.0f};
    FVector2 m_RelativeSize {0.0f, 0.0f};
    bool m_bAutoSize = true;
};

class ImCanvasPanel : public ImPanelWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImCanvasPanel"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImCanvasPanel();
    virtual ~ImCanvasPanel() = default;

    ImCanvasPanelSlot* AddChild(const Ptr& child, const FVector2& relativePosition = FVector2(0.0f, 0.0f));
    ImCanvasPanelSlot* AddChildAt(const Ptr& child, const FVector2& relativePosition);
    ImCanvasPanelSlot* AddChildAt(
        const Ptr& child,
        const FVector2& relativePosition,
        const FVector2& relativeSize);
    void AddChildWithSlot(const Ptr& child, std::unique_ptr<ImCanvasPanelSlot> slot = nullptr);

    void SetDesiredSize(const FVector2& desiredSize) { m_DesiredSize = desiredSize; }
    const FVector2& GetDesiredSize() const { return m_DesiredSize; }

    virtual std::unique_ptr<ImSlot> CreateSlot() override;
    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual void Relayout();

private:
    FVector2 m_DesiredSize {100.0f, 100.0f};
};

} // namespace ImWidgetV4
