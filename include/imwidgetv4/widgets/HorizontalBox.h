#pragma once
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/BoxSlot.h>
#include <memory>
#include <vector>

namespace ImWidgetV4 {

/**
 * @brief 水平布局容器
 *
 * ImHorizontalBox 将子控件水平排列（从左到右）。
 * 支持固定大小和比例填充两种布局模式。
 *
 * 特性：
 * - 子控件水平排列
 * - 支持固定大小和比例填充
 * - 支持子控件间距
 * - 支持内边距
 * - 垂直方向拉伸填充
 */
class ImHorizontalBox : public ImPanelWidget {
public:
    ImHorizontalBox();
    virtual ~ImHorizontalBox() = default;

    // ==================== 子控件管理 ====================

    /**
     * @brief 添加子控件（固定大小）
     *
     * 子控件将使用其期望大小，不会自动填充剩余空间。
     *
     * @param child 子控件
     * @param padding 内边距（可选）
     */
    void AddChild(const Ptr& child, const FMargin& padding = FMargin());

    /**
     * @brief 添加子控件（比例填充）
     *
     * 子控件将按指定的填充系数分配剩余空间。
     *
     * @param child 子控件
     * @param fillCoefficient 填充系数（> 0）
     * @param padding 内边距（可选）
     */
    void AddChildFill(const Ptr& child, float fillCoefficient = 1.0f, const FMargin& padding = FMargin());

    /**
     * @brief 添加子控件（自定义 Slot）
     *
     * 允许完全自定义 Slot 的属性。
     *
     * @param child 子控件
     * @param slot BoxSlot（如果为 nullptr，则创建默认 Slot）
     */
    void AddChildWithSlot(const Ptr& child, std::unique_ptr<ImBoxSlot> slot = nullptr);

    // ==================== 布局属性 ====================

    /**
     * @brief 设置子控件间距
     * @param spacing 间距（像素）
     */
    void SetSpacing(float spacing) { m_Spacing = spacing; }

    /**
     * @brief 获取子控件间距
     * @return 间距（像素）
     */
    float GetSpacing() const { return m_Spacing; }

    // ==================== 重写基类方法 ====================

    /**
     * @brief 创建 Slot（重写以返回 ImBoxSlot）
     * @return ImBoxSlot 指针
     */
    virtual std::unique_ptr<ImSlot> CreateSlot() override;

    /**
     * @brief 绘制控件
     * @param paintContext 绘制上下文
     */
    virtual void Paint(const FPaintContext& paintContext) override;

    /**
     * @brief 获取最小尺寸
     * @return 最小尺寸
     */
    virtual FVector2 GetMinSize() const override;

    /**
     * @brief 重新布局
     *
     * 计算并设置所有子控件的位置和大小。
     */
    virtual void Relayout();

private:
    float m_Spacing = 0.0f;  // 子控件间距

    /**
     * @brief 计算期望大小
     *
     * 遍历所有子控件，计算容器的期望大小。
     *
     * @return 期望大小
     */
    FVector2 ComputeDesiredSize() const;

    /**
     * @brief 排列子控件
     *
     * 根据容器大小和子控件的填充系数，计算并设置每个子控件的位置和大小。
     */
    void ArrangeChildren();
};

} // namespace ImWidgetV4
