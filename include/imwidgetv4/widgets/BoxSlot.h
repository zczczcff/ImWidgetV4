#pragma once
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/core/Types.h>

namespace ImWidgetV4 {

/**
 * @brief Box 布局的 Slot
 *
 * 用于 HorizontalBox 和 VerticalBox 的子控件布局。
 * 支持固定大小和比例填充两种模式。
 */
class ImBoxSlot : public ImPaddingSlot {
public:
    /**
     * @brief 构造函数
     */
    ImBoxSlot();

    /**
     * @brief 虚析构函数
     */
    virtual ~ImBoxSlot() = default;

    /**
     * @brief 设置填充系数
     *
     * - 0.0f: 使用子控件的期望大小（固定大小）
     * - > 0.0f: 按比例填充剩余空间
     *
     * 例如：两个子控件的填充系数分别为 1.0 和 2.0，
     * 则它们将按 1:2 的比例分配剩余空间。
     *
     * @param coefficient 填充系数
     */
    void SetFillCoefficient(float coefficient) { m_FillCoefficient = coefficient; }

    /**
     * @brief 获取填充系数
     * @return 填充系数
     */
    float GetFillCoefficient() const { return m_FillCoefficient; }

    /**
     * @brief 设置是否自动填充
     *
     * 这是一个便捷方法，等价于 SetFillCoefficient(bAutoFill ? 1.0f : 0.0f)
     *
     * @param bAutoFill 是否自动填充
     */
    void SetAutoFill(bool bAutoFill) { m_FillCoefficient = bAutoFill ? 1.0f : 0.0f; }

    /**
     * @brief 获取是否自动填充
     * @return 是否自动填充
     */
    bool IsAutoFill() const { return m_FillCoefficient > 0.0f; }

private:
    float m_FillCoefficient = 0.0f;  // 填充系数（0 = 固定大小，> 0 = 比例填充）
};

} // namespace ImWidgetV4
