#pragma once
#include <imwidgetv4/core/ReflectableObject.h>
#include <imwidgetv4/core/Types.h>

namespace ImWidgetV4 {

class ImWidget;

/**
 * @brief Slot 基类
 *
 * Slot 负责管理子控件的布局信息。
 * 它将布局逻辑与控件逻辑分离，使得容器控件更加灵活和可复用。
 *
 * 注意：Slot 不持有子控件，子控件由父控件的 m_Children 统一管理。
 */
class ImSlot : public ReflectableObject {
    DECLARE_OBJECT_WITH_PARENT(ImSlot, ReflectableObject)
    registrar
        .RegisterProperty(PropertyType::Vec2, "SlotPosition", &ImSlot::m_SlotPosition, "Slot top-left position")
        .RegisterProperty(PropertyType::Vec2, "SlotSize", &ImSlot::m_SlotSize, "Slot size");
    END_DECLARE_OBJECT()

public:
    /**
     * @brief 构造函数
     */
    ImSlot();

    /**
     * @brief 虚析构函数
     */
    virtual ~ImSlot();

    /**
     * @brief 设置 Slot 的位置
     * @param position Slot 位置
     */
    void SetSlotPosition(const FVector2& position) { m_SlotPosition = position; }

    /**
     * @brief 获取 Slot 的位置
     * @return Slot 位置
     */
    const FVector2& GetSlotPosition() const { return m_SlotPosition; }

    /**
     * @brief 设置 Slot 的大小
     * @param size Slot 大小
     */
    void SetSlotSize(const FVector2& size) { m_SlotSize = size; }

    /**
     * @brief 获取 Slot 的大小
     * @return Slot 大小
     */
    const FVector2& GetSlotSize() const { return m_SlotSize; }

    /**
     * @brief 应用布局到子控件
     *
     * 根据 Slot 的位置和大小，计算并设置子控件的实际位置和大小。
     * 子类可以重写此方法以实现自定义布局逻辑（如添加内边距）。
     *
     * @param child 要布局的子控件
     */
    virtual void ApplyLayout(ImWidget* child);

protected:
    FVector2 m_SlotPosition;    // Slot 位置
    FVector2 m_SlotSize;        // Slot 大小
};

/**
 * @brief 带内边距的 Slot
 *
 * 在基础 Slot 的基础上添加内边距功能。
 * 子控件的实际可用空间 = Slot 大小 - 内边距
 */
class ImPaddingSlot : public ImSlot {
    DECLARE_OBJECT_WITH_PARENT(ImPaddingSlot, ImSlot)
    registrar
        .RegisterProperty(PropertyType::Float, "PaddingLeft", &ImPaddingSlot::PaddingLeft, "Left padding")
        .RegisterProperty(PropertyType::Float, "PaddingRight", &ImPaddingSlot::PaddingRight, "Right padding")
        .RegisterProperty(PropertyType::Float, "PaddingTop", &ImPaddingSlot::PaddingTop, "Top padding")
        .RegisterProperty(PropertyType::Float, "PaddingBottom", &ImPaddingSlot::PaddingBottom, "Bottom padding");
    END_DECLARE_OBJECT()

public:
    /**
     * @brief 构造函数
     */
    ImPaddingSlot();

    /**
     * @brief 虚析构函数
     */
    virtual ~ImPaddingSlot() = default;

    /**
     * @brief 应用布局（考虑内边距）
     * @param child 要布局的子控件
     */
    virtual void ApplyLayout(ImWidget* child) override;

    // 内边距属性
    float PaddingLeft = 0.0f;
    float PaddingRight = 0.0f;
    float PaddingTop = 0.0f;
    float PaddingBottom = 0.0f;
};

} // namespace ImWidgetV4
