#include <imwidgetv4/widgets/PanelWidget.h>
#include <algorithm>

namespace ImWidgetV4 {

ImPanelWidget::ImPanelWidget()
    : ImWidget()
{
}

ImPanelWidget::~ImPanelWidget() {
    // 删除所有 Slot
    for (auto slot : m_Slots) {
        if (slot) {
            // 删除子控件
            if (slot->GetContent()) {
                delete slot->GetContent();
            }
            delete slot;
        }
    }
    m_Slots.clear();
}

ImSlot* ImPanelWidget::CreateSlot(ImWidget* content) {
    // 默认创建基础 Slot
    return new ImSlot(content, this);
}

ImSlot* ImPanelWidget::GetSlotAt(int index) {
    if (index >= 0 && index < static_cast<int>(m_Slots.size())) {
        return m_Slots[index];
    }
    return nullptr;
}

const ImSlot* ImPanelWidget::GetSlotAt(int index) const {
    if (index >= 0 && index < static_cast<int>(m_Slots.size())) {
        return m_Slots[index];
    }
    return nullptr;
}

void ImPanelWidget::SetChildAt(int index, ImWidget* child, bool bDeleteOld) {
    // 确保 Slot 列表足够大
    while (static_cast<int>(m_Slots.size()) <= index) {
        m_Slots.push_back(nullptr);
    }

    // 获取或创建 Slot
    ImSlot* slot = m_Slots[index];
    if (!slot) {
        // 创建新 Slot
        slot = CreateSlot(child);
        m_Slots[index] = slot;
    } else {
        // 删除旧的子控件
        if (bDeleteOld && slot->GetContent()) {
            delete slot->GetContent();
        }
        // 设置新的子控件
        slot->SetContent(child);
    }
}

ImWidget* ImPanelWidget::GetChildAt(int index) {
    ImSlot* slot = GetSlotAt(index);
    if (slot) {
        return slot->GetContent();
    }
    return nullptr;
}

const ImWidget* ImPanelWidget::GetChildAt(int index) const {
    const ImSlot* slot = GetSlotAt(index);
    if (slot) {
        return slot->GetContent();
    }
    return nullptr;
}

void ImPanelWidget::RenderChild(const FPaintContext& paintContext) {
    for (auto slot : m_Slots) {
        if (slot) {
            slot->Render(paintContext);
        }
    }
}

bool ImPanelWidget::BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) {
    // 检查位置是否在此控件内
    if (!m_Geometry.Contains(position)) {
        return false;
    }

    // 先检查子控件（从后往前，后面的控件在上层）
    // 注意：子控件可能不是由 shared_ptr 管理的，所以不能调用它们的 BuildHitTestPath
    // 我们只检查子控件的几何信息来判断是否命中
    for (int i = static_cast<int>(m_Slots.size()) - 1; i >= 0; --i) {
        ImSlot* slot = m_Slots[i];
        if (slot && slot->GetContent()) {
            ImWidget* child = slot->GetContent();
            if (child->IsVisible() && child->IsHitTestVisible()) {
                // 检查子控件的几何信息是否包含该位置
                if (child->GetGeometry().Contains(position)) {
                    // 子控件命中，将此控件（父控件）添加到路径
                    outPath.push_back(shared_from_this());
                    return true;
                }
            }
        }
    }

    // 如果没有子控件命中，返回此控件
    outPath.push_back(shared_from_this());
    return true;
}

} // namespace ImWidgetV4
