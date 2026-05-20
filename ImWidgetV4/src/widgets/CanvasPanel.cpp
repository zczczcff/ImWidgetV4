#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>
#include <algorithm>

namespace ImWidgetV4 {

const Reflection::FTypeDesc& ImCanvasPanelSlot::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImCanvasPanelSlot, FVector2, &ImCanvasPanelSlot::m_RelativePosition>(
            "ImCanvasPanelSlot", "RelativePosition", Reflection::EPropertyKind::Vec2, "FVector2", "Relative position"),
        Reflection::MakeMemberProperty<ImCanvasPanelSlot, FVector2, &ImCanvasPanelSlot::m_RelativeSize>(
            "ImCanvasPanelSlot", "RelativeSize", Reflection::EPropertyKind::Vec2, "FVector2", "Relative size"),
        Reflection::MakeMemberProperty<ImCanvasPanelSlot, bool, &ImCanvasPanelSlot::m_bAutoSize>(
            "ImCanvasPanelSlot", "AutoSize", Reflection::EPropertyKind::Bool, "bool", "Whether the slot auto sizes to its child")
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImCanvasPanelSlot",
        &ImSlot::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

const Reflection::FTypeDesc& ImCanvasPanel::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImCanvasPanel, FVector2, &ImCanvasPanel::m_DesiredSize>(
            "ImCanvasPanel", "DesiredSize", Reflection::EPropertyKind::Vec2, "FVector2", "Desired panel size")
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImCanvasPanel",
        &ImPanelWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& ImCanvasPanelSlotReflectionTypeDesc = ImCanvasPanelSlot::StaticTypeDesc();
const Reflection::FTypeDesc& ImCanvasPanelReflectionTypeDesc = ImCanvasPanel::StaticTypeDesc();

} // namespace

ImCanvasPanel::ImCanvasPanel()
    : ImPanelWidget()
{
}

ImCanvasPanelSlot* ImCanvasPanel::AddChild(const Ptr& child, const FVector2& relativePosition)
{
    auto slot = std::make_unique<ImCanvasPanelSlot>();
    slot->SetRelativePosition(relativePosition);
    slot->SetAutoSize(true);
    ImCanvasPanelSlot* slotPtr = slot.get();
    AddChildWithSlot(child, std::move(slot));
    return slotPtr;
}

ImCanvasPanelSlot* ImCanvasPanel::AddChildAt(const Ptr& child, const FVector2& relativePosition)
{
    return AddChild(child, relativePosition);
}

ImCanvasPanelSlot* ImCanvasPanel::AddChildAt(
    const Ptr& child,
    const FVector2& relativePosition,
    const FVector2& relativeSize)
{
    auto slot = std::make_unique<ImCanvasPanelSlot>();
    slot->SetRelativePosition(relativePosition);
    slot->SetRelativeSize(relativeSize);
    slot->SetAutoSize(false);
    ImCanvasPanelSlot* slotPtr = slot.get();
    AddChildWithSlot(child, std::move(slot));
    return slotPtr;
}

void ImCanvasPanel::AddChildWithSlot(const Ptr& child, std::unique_ptr<ImCanvasPanelSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImCanvasPanelSlot>();
    }

    AddSlot(child, std::move(slot));
}

std::unique_ptr<ImSlot> ImCanvasPanel::CreateSlot()
{
    return std::make_unique<ImCanvasPanelSlot>();
}

void ImCanvasPanel::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();
    paintContext.DrawContext_.PushClipRect(m_Geometry.GetMin(), m_Geometry.GetMax(), true);
    RenderChildren(paintContext);
    paintContext.DrawContext_.PopClipRect();
}

FVector2 ImCanvasPanel::GetMinSize() const
{
    return m_DesiredSize;
}

void ImCanvasPanel::Relayout()
{
    const auto& children = GetChildren();
    for (size_t index = 0; index < children.size(); ++index) {
        const Ptr& child = children[index];
        auto* slot = dynamic_cast<ImCanvasPanelSlot*>(GetSlotAt(static_cast<int>(index)));
        if (!child || !slot) {
            continue;
        }

        const FVector2 childPosition = FVector2(
            m_Geometry.Position.X + slot->GetRelativePosition().X * m_Geometry.Size.X,
            m_Geometry.Position.Y + slot->GetRelativePosition().Y * m_Geometry.Size.Y);

        FVector2 childSize = child->GetMinSize();
        if (!slot->GetAutoSize()) {
            childSize = FVector2(
                std::max(0.0f, slot->GetRelativeSize().X * m_Geometry.Size.X),
                std::max(0.0f, slot->GetRelativeSize().Y * m_Geometry.Size.Y));
        }

        slot->SetSlotPosition(childPosition);
        slot->SetSlotSize(childSize);
        slot->ApplyLayout(child.get());
    }
}

} // namespace ImWidgetV4
