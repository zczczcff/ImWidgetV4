#pragma once

#include <imwidgetv4/widgets/BoxSlot.h>
#include <imwidgetv4\widgets/PanelWidget.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

class ImHorizontalBox : public ImPanelWidget {
    DECLARE_OBJECT_WITH_PARENT(ImHorizontalBox, ImPanelWidget)
    registrar
        .RegisterProperty(PropertyType::Float, "Spacing", &ImHorizontalBox::m_Spacing, "Horizontal spacing between child widgets");
    END_DECLARE_OBJECT()

public:
    ImHorizontalBox();
    virtual ~ImHorizontalBox() = default;

    void AddChild(const Ptr& child, const FMargin& padding = FMargin());
    void AddChildFill(const Ptr& child, float fillCoefficient = 1.0f, const FMargin& padding = FMargin());
    void AddChildWithSlot(const Ptr& child, std::unique_ptr<ImBoxSlot> slot = nullptr);
    void InsertChild(int index, const Ptr& child, const FMargin& padding = FMargin());
    void InsertChildFill(int index, const Ptr& child, float fillCoefficient = 1.0f, const FMargin& padding = FMargin());
    void InsertChildWithSlot(int index, const Ptr& child, std::unique_ptr<ImBoxSlot> slot = nullptr);

    void SetSpacing(float spacing) { m_Spacing = spacing; }
    float GetSpacing() const { return m_Spacing; }

    virtual std::unique_ptr<ImSlot> CreateSlot() override;
    virtual void Paint(const FPaintContext& paintContext) override;
    virtual FVector2 GetMinSize() const override;
    virtual void Relayout();

private:
    FVector2 ComputeDesiredSize() const;
    void ArrangeChildren();

    float m_Spacing = 0.0f;
};

} // namespace ImWidgetV4
