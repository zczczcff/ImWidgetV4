#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/reflection/ReflectionRegistry.h>

#include <algorithm>

namespace ImWidgetV4 {

const Reflection::FTypeDesc& ImPanelWidget::StaticTypeDesc()
{
    static const Reflection::FTypeDesc typeDesc {
        "ImPanelWidget",
        &ImWidget::StaticTypeDesc(),
        nullptr,
        0
    };
    static const Reflection::FAutoTypeRegistration registration(&typeDesc);
    (void)registration;
    return typeDesc;
}

namespace {

const Reflection::FTypeDesc& ImPanelWidgetReflectionTypeDesc = ImPanelWidget::StaticTypeDesc();

} // namespace

ImPanelWidget::ImPanelWidget()
    : ImWidget()
{
}

ImPanelWidget::~ImPanelWidget() = default;

std::unique_ptr<ImSlot> ImPanelWidget::CreateSlot()
{
    return std::make_unique<ImSlot>();
}

void ImPanelWidget::AddSlot(const Ptr& child, std::unique_ptr<ImSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = CreateSlot();
    }

    AddChild(child);
    m_Slots.push_back(std::move(slot));
}

void ImPanelWidget::InsertSlot(int index, const Ptr& child, std::unique_ptr<ImSlot> slot)
{
    if (!child) {
        return;
    }

    if (!slot) {
        slot = CreateSlot();
    }

    const int clampedIndex = std::clamp(index, 0, static_cast<int>(m_Children.size()));
    InsertChildAt(clampedIndex, child);
    m_Slots.insert(m_Slots.begin() + clampedIndex, std::move(slot));
}

ImSlot* ImPanelWidget::GetSlotAt(int index)
{
    if (index >= 0 && index < static_cast<int>(m_Slots.size())) {
        return m_Slots[static_cast<std::size_t>(index)].get();
    }
    return nullptr;
}

const ImSlot* ImPanelWidget::GetSlotAt(int index) const
{
    if (index >= 0 && index < static_cast<int>(m_Slots.size())) {
        return m_Slots[static_cast<std::size_t>(index)].get();
    }
    return nullptr;
}

ImSlot* ImPanelWidget::GetSlotForChild(const Ptr& child)
{
    if (!child) {
        return nullptr;
    }

    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        const int index = static_cast<int>(std::distance(m_Children.begin(), it));
        return GetSlotAt(index);
    }

    return nullptr;
}

const ImSlot* ImPanelWidget::GetSlotForChild(const Ptr& child) const
{
    if (!child) {
        return nullptr;
    }

    auto it = std::find(m_Children.begin(), m_Children.end(), child);
    if (it != m_Children.end()) {
        const int index = static_cast<int>(std::distance(m_Children.begin(), it));
        return GetSlotAt(index);
    }

    return nullptr;
}

bool ImPanelWidget::RemoveChild(const Ptr& child)
{
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

void ImPanelWidget::RenderChildren(const FPaintContext& paintContext)
{
    for (const Ptr& child : m_Children) {
        if (child && child->IsVisible()) {
            child->Paint(paintContext);
        }
    }
}

} // namespace ImWidgetV4
