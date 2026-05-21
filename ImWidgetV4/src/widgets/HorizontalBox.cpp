#include <imwidgetv4/widgets/HorizontalBox.h>

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

const Reflection::FTypeDesc& ImHorizontalBox::StaticTypeDesc()
{
    static const Reflection::FPropertyDesc properties[] = {
        Reflection::MakeMemberProperty<ImHorizontalBox, float, &ImHorizontalBox::m_Spacing>(
            "ImHorizontalBox", "Spacing", Reflection::EPropertyKind::Float, "float", "Horizontal spacing between child widgets")
    };

    static const Reflection::FTypeDesc typeDesc {
        "ImHorizontalBox",
        &ImPanelWidget::StaticTypeDesc(),
        properties,
        sizeof(properties) / sizeof(properties[0])
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& ImHorizontalBoxReflectionTypeDesc = ImHorizontalBox::StaticTypeDesc();

} // namespace

ImHorizontalBox::ImHorizontalBox()
    : ImPanelWidget()
    , m_Spacing(0.0f)
{
}

void ImHorizontalBox::AddChild(const Ptr& child, const FMargin& padding)
{
    AddChildWithSlot(child, MakeBoxSlot(0.0f, padding));
}

void ImHorizontalBox::AddChildFill(const Ptr& child, float fillCoefficient, const FMargin& padding)
{
    AddChildWithSlot(child, MakeBoxSlot(fillCoefficient, padding));
}

void ImHorizontalBox::AddChildWithSlot(const Ptr& child, std::unique_ptr<ImBoxSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImBoxSlot>();
    }

    AddSlot(child, std::move(slot));
}

void ImHorizontalBox::InsertChild(int index, const Ptr& child, const FMargin& padding)
{
    InsertChildWithSlot(index, child, MakeBoxSlot(0.0f, padding));
}

void ImHorizontalBox::InsertChildFill(int index, const Ptr& child, float fillCoefficient, const FMargin& padding)
{
    InsertChildWithSlot(index, child, MakeBoxSlot(fillCoefficient, padding));
}

void ImHorizontalBox::InsertChildWithSlot(int index, const Ptr& child, std::unique_ptr<ImBoxSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = std::make_unique<ImBoxSlot>();
    }

    InsertSlot(index, child, std::move(slot));
}

std::unique_ptr<ImSlot> ImHorizontalBox::CreateSlot()
{
    return std::make_unique<ImBoxSlot>();
}

void ImHorizontalBox::Paint(const FPaintContext& paintContext)
{
    if (!m_bVisible) {
        return;
    }

    Relayout();
    RenderChildren(paintContext);
}

FVector2 ImHorizontalBox::GetMinSize() const
{
    return ComputeDesiredSize();
}

void ImHorizontalBox::Relayout()
{
    ArrangeChildren();
}

FVector2 ImHorizontalBox::ComputeDesiredSize() const
{
    const auto& children = GetChildren();
    if (children.empty()) {
        return FVector2(DefaultEmptyBoxMinWidth, DefaultEmptyBoxMinHeight);
    }

    float totalWidth = 0.0f;
    float maxHeight = 0.0f;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        const ImBoxSlot* slot = dynamic_cast<const ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        const FVector2 childMinSize = child->GetMinSize();
        const float childWidth = childMinSize.X + slot->PaddingLeft + slot->PaddingRight;
        const float childHeight = childMinSize.Y + slot->PaddingTop + slot->PaddingBottom;

        totalWidth += childWidth;
        maxHeight = std::max(maxHeight, childHeight);

        if (i + 1 < children.size()) {
            totalWidth += m_Spacing;
        }
    }

    return FVector2(totalWidth, maxHeight);
}

void ImHorizontalBox::ArrangeChildren()
{
    const auto& children = GetChildren();
    if (children.empty()) {
        return;
    }

    const FGeometry& geometry = GetGeometry();
    const float containerWidth = geometry.Size.X;
    const float containerHeight = geometry.Size.Y;

    float totalFixedWidth = 0.0f;
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
            totalFixedWidth += childMinSize.X + slot->PaddingLeft + slot->PaddingRight;
        }

        if (i + 1 < children.size()) {
            totalFixedWidth += m_Spacing;
        }
    }

    const float remainingWidth = std::max(0.0f, containerWidth - totalFixedWidth);
    float currentX = geometry.Position.X;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        ImBoxSlot* slot = dynamic_cast<ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        float childWidth = 0.0f;
        if (slot->GetFillCoefficient() > 0.0f && totalFillCoefficient > 0.0f) {
            childWidth = (remainingWidth * slot->GetFillCoefficient() / totalFillCoefficient)
                - slot->PaddingLeft - slot->PaddingRight;
            childWidth = std::max(0.0f, childWidth);
        } else {
            childWidth = child->GetMinSize().X;
        }

        float childHeight = containerHeight - slot->PaddingTop - slot->PaddingBottom;
        childHeight = std::max(0.0f, childHeight);

        slot->SetSlotPosition(FVector2(currentX, geometry.Position.Y));
        slot->SetSlotSize(FVector2(childWidth + slot->PaddingLeft + slot->PaddingRight, containerHeight));
        slot->ApplyLayout(child.get());

        currentX += childWidth + slot->PaddingLeft + slot->PaddingRight;
        if (i + 1 < children.size()) {
            currentX += m_Spacing;
        }
    }
}

} // namespace ImWidgetV4
