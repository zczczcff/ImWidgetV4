#include <imwidgetv4/widgets/PanelWidget.h>
#include <algorithm>

namespace ImWidgetV4 {

ImPanelWidget::ImPanelWidget()
    : ImWidget()
{
}

ImPanelWidget::~ImPanelWidget() {
    // Slot 使用 unique_ptr 自动释放
    // 子控件由基类的 m_Children 管理
}

std::unique_ptr<ImSlot> ImPanelWidget::CreateSlot() {
    // 默认创建基础 Slot
    return std::make_unique<ImSlot>();
}

void ImPanelWidget::AddSlot(const Ptr& child, std::unique_ptr<ImSlot> slot) {
    if (!child) {
        return;
    }

    // 如果没有提供 Slot，则创建一个
    if (!slot) {
        slot = CreateSlot();
    }

    // 通过基类添加子控件
    AddChild(child);

    // 添加 Slot
    m_Slots.push_back(std::move(slot));
}

ImSlot* ImPanelWidget::GetSlotAt(int index) {
    if (index >= 0 && index < static_cast<int>(m_Slots.size())) {
        return m_Slots[index].get();
    }
    return nullptr;
}

const ImSlot* ImPanelWidget::GetSlotAt(int index) const {
    if (index >= 0 && index < static_cast<int>(m_Slots.size())) {
        return m_Slots[index].get();
    }
    return nullptr;
}

ImSlot* ImPanelWidget::GetSlotForChild(const Ptr& child) {
    if (!child) {
        return nullptr;
    }

    // 查找子控件在 m_Children 中的索引
    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        int index = static_cast<int>(std::distance(m_Children.begin(), it));
        return GetSlotAt(index);
    }

    return nullptr;
}

const ImSlot* ImPanelWidget::GetSlotForChild(const Ptr& child) const {
    if (!child) {
        return nullptr;
    }

    // 查找子控件在 m_Children 中的索引
    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        int index = static_cast<int>(std::distance(m_Children.begin(), it));
        return GetSlotAt(index);
    }

    return nullptr;
}

bool ImPanelWidget::RemoveChild(const Ptr& child) {
    if (!child) {
        return false;
    }

    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it == m_Children.end()) {
        return false;
    }

    const std::size_t index = static_cast<std::size_t>(std::distance(m_Children.begin(), it));
    if (!ImWidget::RemoveChild(child)) {
        return false;
    }

    if (index < m_Slots.size()) {
        m_Slots.erase(m_Slots.begin() + static_cast<std::ptrdiff_t>(index));
    }

    return true;
}

void ImPanelWidget::RenderChildren(const FPaintContext& paintContext) {
    // 遍历所有子控件并渲染
    for (const Ptr& child : m_Children) {
        if (child && child->IsVisible()) {
            child->Paint(paintContext);
        }
    }
}

} // namespace ImWidgetV4
