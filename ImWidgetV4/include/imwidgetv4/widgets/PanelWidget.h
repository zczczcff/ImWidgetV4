#pragma once
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/core/Slot.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

/**
 * @brief 容器控件基类
 *
 * ImPanelWidget 是所有容器控件的基类，提供子控件管理和 Slot 系统。
 * 它使用 Slot 来管理子控件的布局，将布局逻辑与控件逻辑分离。
 *
 * 注意：子控件统一由基类的 m_Children 管理，Slot 只存储布局信息。
 */
class ImPanelWidget : public ImWidget {
public:
    static const Reflection::FTypeDesc& StaticTypeDesc();
    std::string GetTypeName() const override { return "ImPanelWidget"; }
    const Reflection::FTypeDesc& GetTypeDesc() const override { return StaticTypeDesc(); }

    ImPanelWidget();
    virtual ~ImPanelWidget();

    /**
     * @brief 创建 Slot
     *
     * 子类可以重写此方法以创建特定类型的 Slot。
     * 例如，ImButton 重写此方法返回 ImPaddingSlot。
     *
     * @return 创建的 Slot 指针
     */
    virtual std::unique_ptr<ImSlot> CreateSlot();

    /**
     * @brief 添加子控件和对应的 Slot
     * @param child 子控件
     * @param slot Slot（如果为 nullptr，则使用 CreateSlot 创建）
     */
    void AddSlot(const Ptr& child, std::unique_ptr<ImSlot> slot = nullptr);
    void InsertSlot(int index, const Ptr& child, std::unique_ptr<ImSlot> slot = nullptr);

    /**
     * @brief 获取指定索引的 Slot
     * @param index Slot 索引
     * @return Slot 指针，如果索引无效返回 nullptr
     */
    ImSlot* GetSlotAt(int index);

    /**
     * @brief 获取指定索引的 Slot（const 版本）
     * @param index Slot 索引
     * @return Slot 指针，如果索引无效返回 nullptr
     */
    const ImSlot* GetSlotAt(int index) const;

    /**
     * @brief 获取子控件对应的 Slot
     * @param child 子控件
     * @return Slot 指针，如果未找到返回 nullptr
     */
    ImSlot* GetSlotForChild(const Ptr& child);

    /**
     * @brief 获取子控件对应的 Slot（const 版本）
     * @param child 子控件
     * @return Slot 指针，如果未找到返回 nullptr
     */
    const ImSlot* GetSlotForChild(const Ptr& child) const;
    virtual bool RemoveChild(const Ptr& child) override;

    /**
     * @brief 渲染所有子控件
     * @param paintContext 绘制上下文
     */
    void RenderChildren(const FPaintContext& paintContext);

    /**
     * @brief 获取 Slot 数量
     * @return Slot 数量
     */
    int GetSlotCount() const { return static_cast<int>(m_Slots.size()); }

protected:
    // Slot 列表（只存储布局信息，不持有子控件）
    std::vector<std::unique_ptr<ImSlot>> m_Slots;
};

} // namespace ImWidgetV4
