#include <imwidgetv4/widgets/VerticalBox.h>
#include <imwidgetv4/core/DrawContext.h>
#include <algorithm>

namespace ImWidgetV4 {

ImVerticalBox::ImVerticalBox()
    : ImPanelWidget()
    , m_Spacing(0.0f)
{
}

// ==================== 子控件管理 ====================

void ImVerticalBox::AddChild(const Ptr& child, const FMargin& padding) {
    if (!child) {
        return;
    }

    // 创建 BoxSlot
    auto slot = std::make_unique<ImBoxSlot>();
    slot->PaddingLeft = padding.Left;
    slot->PaddingRight = padding.Right;
    slot->PaddingTop = padding.Top;
    slot->PaddingBottom = padding.Bottom;
    slot->SetFillCoefficient(0.0f);  // 固定大小

    // 添加子控件和 Slot
    AddSlot(child, std::move(slot));
}

void ImVerticalBox::AddChildFill(const Ptr& child, float fillCoefficient, const FMargin& padding) {
    if (!child) {
        return;
    }

    // 创建 BoxSlot
    auto slot = std::make_unique<ImBoxSlot>();
    slot->PaddingLeft = padding.Left;
    slot->PaddingRight = padding.Right;
    slot->PaddingTop = padding.Top;
    slot->PaddingBottom = padding.Bottom;
    slot->SetFillCoefficient(fillCoefficient);  // 比例填充

    // 添加子控件和 Slot
    AddSlot(child, std::move(slot));
}

void ImVerticalBox::AddChildWithSlot(const Ptr& child, std::unique_ptr<ImBoxSlot> slot) {
    if (!child) {
        return;
    }

    // 如果没有提供 Slot，则创建默认 Slot
    if (!slot) {
        slot = std::make_unique<ImBoxSlot>();
    }

    // 添加子控件和 Slot
    AddSlot(child, std::move(slot));
}

// ==================== 重写基类方法 ====================

std::unique_ptr<ImSlot> ImVerticalBox::CreateSlot() {
    // 创建 BoxSlot
    return std::make_unique<ImBoxSlot>();
}

void ImVerticalBox::Paint(const FPaintContext& paintContext) {
    if (!m_bVisible) {
        return;
    }

    // 1. 重新布局（如果需要）
    Relayout();

    // 2. 绘制子控件
    RenderChildren(paintContext);
}

FVector2 ImVerticalBox::GetMinSize() const {
    return ComputeDesiredSize();
}

void ImVerticalBox::Relayout() {
    ArrangeChildren();
}

// ==================== 内部方法 ====================

FVector2 ImVerticalBox::ComputeDesiredSize() const {
    const auto& children = GetChildren();
    if (children.empty()) {
        return FVector2(0.0f, 0.0f);
    }

    float totalHeight = 0.0f;
    float maxWidth = 0.0f;

    // 遍历所有子控件
    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        const ImBoxSlot* slot = dynamic_cast<const ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        // 获取子控件的期望大小
        FVector2 childMinSize = child->GetMinSize();

        // 加上内边距
        float childHeight = childMinSize.Y + slot->PaddingTop + slot->PaddingBottom;
        float childWidth = childMinSize.X + slot->PaddingLeft + slot->PaddingRight;

        // 累加高度
        totalHeight += childHeight;

        // 更新最大宽度
        maxWidth = std::max(maxWidth, childWidth);

        // 添加间距（除了最后一个子控件）
        if (i < children.size() - 1) {
            totalHeight += m_Spacing;
        }
    }

    return FVector2(maxWidth, totalHeight);
}

void ImVerticalBox::ArrangeChildren() {
    const auto& children = GetChildren();
    if (children.empty()) {
        return;
    }

    // 获取容器的几何信息
    const FGeometry& geometry = GetGeometry();
    float containerHeight = geometry.Size.Y;
    float containerWidth = geometry.Size.X;

    // 计算总的固定高度和填充系数
    float totalFixedHeight = 0.0f;
    float totalFillCoefficient = 0.0f;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        const ImBoxSlot* slot = dynamic_cast<const ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        if (slot->GetFillCoefficient() > 0.0f) {
            // 填充子控件
            totalFillCoefficient += slot->GetFillCoefficient();
        } else {
            // 固定大小子控件
            FVector2 childMinSize = child->GetMinSize();
            totalFixedHeight += childMinSize.Y + slot->PaddingTop + slot->PaddingBottom;
        }

        // 添加间距（除了最后一个子控件）
        if (i < children.size() - 1) {
            totalFixedHeight += m_Spacing;
        }
    }

    // 计算剩余空间
    float remainingHeight = std::max(0.0f, containerHeight - totalFixedHeight);

    // 排列子控件
    float currentY = geometry.Position.Y;

    for (size_t i = 0; i < children.size(); ++i) {
        const Ptr& child = children[i];
        ImBoxSlot* slot = dynamic_cast<ImBoxSlot*>(GetSlotAt(static_cast<int>(i)));

        if (!child || !slot) {
            continue;
        }

        // 计算子控件的高度
        float childHeight;
        if (slot->GetFillCoefficient() > 0.0f) {
            // 填充子控件：按比例分配剩余空间
            childHeight = (remainingHeight * slot->GetFillCoefficient() / totalFillCoefficient)
                        - slot->PaddingTop - slot->PaddingBottom;
            childHeight = std::max(0.0f, childHeight);
        } else {
            // 固定大小子控件：使用期望大小
            FVector2 childMinSize = child->GetMinSize();
            childHeight = childMinSize.Y;
        }

        // 计算子控件的宽度（水平方向拉伸填充）
        float childWidth = containerWidth - slot->PaddingLeft - slot->PaddingRight;
        childWidth = std::max(0.0f, childWidth);

        // 设置 Slot 的位置和大小
        slot->SetSlotPosition(FVector2(geometry.Position.X, currentY));
        slot->SetSlotSize(FVector2(containerWidth, childHeight + slot->PaddingTop + slot->PaddingBottom));

        // 应用布局（计算子控件的实际位置和大小）
        slot->ApplyLayout(child.get());

        // 移动到下一个子控件的位置
        currentY += childHeight + slot->PaddingTop + slot->PaddingBottom;

        // 添加间距（除了最后一个子控件）
        if (i < children.size() - 1) {
            currentY += m_Spacing;
        }
    }
}

} // namespace ImWidgetV4
