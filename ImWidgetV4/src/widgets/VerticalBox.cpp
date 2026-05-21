#include <imwidgetv4/widgets/VerticalBox.h>

#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/reflection/ReflectionBuilder.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>

#include <algorithm>

namespace ImWidgetV4 {

namespace {

constexpr float DefaultEmptyBoxMinWidth = 120.0f;
constexpr float DefaultEmptyBoxMinHeight = 80.0f;

std::unique_ptr<ImBoxSlot> MakeBoxSlot(float fillCoefficient, const FMargin& padding)
{
    auto slot = std::make_unique<ImBoxSlot>();
    slot->PaddingLeft = padding.Left;
    slot->PaddingRight = padding.Right;
    slot->PaddingTop = padding.Top;
    slot->PaddingBottom = padding.Bottom;
    slot->SetFillCoefficient(fillCoefficient);
    return slot;
}

} // namespace

const Reflection::FTypeDesc& ImVerticalBox::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImVerticalBox, float, &ImVerticalBox::m_Spacing>(
            "ImVerticalBox", "Spacing", Reflection::EPropertyKind::Float, "float", "Vertical spacing between child widgets")
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImVerticalBox",
        &ImPanelWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& ImVerticalBoxReflectionTypeDesc = ImVerticalBox::StaticTypeDesc();

} // namespace

ImVerticalBox::ImVerticalBox()
    : ImPanelWidget()
    , m_Spacing(0.0f)
{
}

void ImVerticalBox::AddChild(const Ptr& child, const FMargin& padding)
{
    AddChildWithSlot(child, MakeBoxSlot(0.0f, padding));
}

void ImVerticalBox::AddChildFill(const Ptr& child, float fillCoefficient, const FMargin& padding)
{
    AddChildWithSlot(child, MakeBoxSlot(fillCoefficient, padding));
}

void ImVerticalBox::AddChildWithSlot(const Ptr& child, std::unique_ptr<ImBoxSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImBoxSlot>();
    }

    AddSlot(child, std::move(slot));
}

void ImVerticalBox::InsertChild(int index, const Ptr& child, const FMargin& padding)
{
    InsertChildWithSlot(index, child, MakeBoxSlot(0.0f, padding));
}

void ImVerticalBox::InsertChildFill(int index, const Ptr& child, float fillCoefficient, const FMargin& padding)
{
    InsertChildWithSlot(index, child, MakeBoxSlot(fillCoefficient, padding));
}

void ImVerticalBox::InsertChildWithSlot(int index, const Ptr& child, std::unique_ptr<ImBoxSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImBoxSlot>();
    }

    InsertSlot(index, child, std::move(slot));
}

std::unique_ptr<ImSlot> ImVerticalBox::CreateSlot()
{
    return std::make_unique<ImBoxSlot>();
}

void ImVerticalBox::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();
    RenderChildren(paintContext);
}

FVector2 ImVerticalBox::GetMinSize() const
{
    return ComputeDesiredSize();
}

void ImVerticalBox::Relayout()
{
    ArrangeChildren();
}

FVector2 ImVerticalBox::ComputeDesiredSize() const
{
    const auto& children = GetChildren();
    if (children.empty()) {
        return FVector2(DefaultEmptyBoxMinWidth, DefaultEmptyBoxMinHeight);
    }

    float totalHeight = 0.0f;
    float maxWidth = 0.0f;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        const ImBoxSlot* slot = dynamic_cast<const ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        const FVector2 childMinSize = child->GetMinSize();
        const float childHeight = childMinSize.Y + slot->PaddingTop + slot->PaddingBottom;
        const float childWidth = childMinSize.X + slot->PaddingLeft + slot->PaddingRight;

        totalHeight += childHeight;
        maxWidth = std::max(maxWidth, childWidth);

        if (i + 1 < children.size()) {
            totalHeight += m_Spacing;
        }
    }

    return FVector2(maxWidth, totalHeight);
}

void ImVerticalBox::ArrangeChildren()
{
    const auto& children = GetChildren();
    if (children.empty()) {
        return;
    }

    const FGeometry& geometry = GetGeometry();
    const float containerHeight = geometry.Size.Y;
    const float containerWidth = geometry.Size.X;

    float totalFixedHeight = 0.0f;
    float totalFillCoefficient = 0.0f;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        const ImBoxSlot* slot = dynamic_cast<const ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        if (slot->GetFillCoefficient() > 0.0f) {
            totalFillCoefficient += slot->GetFillCoefficient();
        } else {
            const FVector2 childMinSize = child->GetMinSize();
            totalFixedHeight += childMinSize.Y + slot->PaddingTop + slot->PaddingBottom;
        }

        if (i + 1 < children.size()) {
            totalFixedHeight += m_Spacing;
        }
    }

    const float remainingHeight = std::max(0.0f, containerHeight - totalFixedHeight);
    float currentY = geometry.Position.Y;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        ImBoxSlot* slot = dynamic_cast<ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        float childHeight = 0.0f;
        if (slot->GetFillCoefficient() > 0.0f && totalFillCoefficient > 0.0f) {
            childHeight = (remainingHeight * slot->GetFillCoefficient() / totalFillCoefficient)
                - slot->PaddingTop - slot->PaddingBottom;
            childHeight = std::max(0.0f, childHeight);
        } else {
            childHeight = child->GetMinSize().Y;
        }

        float childWidth = containerWidth - slot->PaddingLeft - slot->PaddingRight;
        childWidth = std::max(0.0f, childWidth);

        slot->SetSlotPosition(FVector2(geometry.Position.X, currentY));
        slot->SetSlotSize(FVector2(containerWidth, childHeight + slot->PaddingTop + slot->PaddingBottom));
        slot->ApplyLayout(child.get());

        currentY += childHeight + slot->PaddingTop + slot->PaddingBottom;
        if (i + 1 < children.size()) {
            currentY += m_Spacing;
        }
    }
}

} // namespace ImWidgetV4
