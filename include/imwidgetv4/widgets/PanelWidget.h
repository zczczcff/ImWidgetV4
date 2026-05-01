#pragma once
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/core/Slot.h>
#include <vector>

namespace ImWidgetV4 {

/**
 * @brief 容器控件基类
 *
 * ImPanelWidget 是所有容器控件的基类，提供子控件管理和 Slot 系统。
 * 它使用 Slot 来管理子控件的布局，将布局逻辑与控件逻辑分离。
 */
class ImPanelWidget : public ImWidget {
public:
    ImPanelWidget();
    virtual ~ImPanelWidget();

    /**
     * @brief 创建 Slot
     *
     * 子类可以重写此方法以创建特定类型的 Slot。
     * 例如，ImButton 重写此方法返回 ImPaddingSlot。
     *
     * @param content 子控件指针
     * @return 创建的 Slot 指针
     */
    virtual ImSlot* CreateSlot(ImWidget* content);

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
     * @brief 设置指定索引的子控件
     * @param index Slot 索引
     * @param child 子控件指针
     * @param bDeleteOld 是否删除旧的子控件
     */
    void SetChildAt(int index, ImWidget* child, bool bDeleteOld = true);

    /**
     * @brief 获取指定索引的子控件
     * @param index Slot 索引
     * @return 子控件指针，如果索引无效返回 nullptr
     */
    ImWidget* GetChildAt(int index);

    /**
     * @brief 获取指定索引的子控件（const 版本）
     * @param index Slot 索引
     * @return 子控件指针，如果索引无效返回 nullptr
     */
    const ImWidget* GetChildAt(int index) const;

    /**
     * @brief 渲染所有子控件
     * @param paintContext 绘制上下文
     */
    void RenderChild(const FPaintContext& paintContext);

    /**
     * @brief 命中测试
     *
     * 检查指定位置是否命中此控件或其子控件。
     * 子控件优先于父控件进行命中测试。
     *
     * @param position 测试位置
     * @param outPath 输出命中路径（从根到叶）
     * @return 如果命中返回 true
     */
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath) override;

    /**
     * @brief 获取 Slot 数量
     * @return Slot 数量
     */
    int GetSlotCount() const { return static_cast<int>(m_Slots.size()); }

protected:
    std::vector<ImSlot*> m_Slots;  // Slot 列表
};

} // namespace ImWidgetV4
