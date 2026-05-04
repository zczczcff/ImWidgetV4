#pragma once
#include <imwidgetv4/widgets/PanelWidget.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <functional>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 按钮控件
 *
 * 可点击的按钮控件，支持多种状态和样式。
 * 作为单子项容器控件，可以包含任何 ImWidget 作为内容（文本、图片、复杂布局等）。
 * 参考 ImWidget 项目的按钮实现设计。
 */
class ImButton : public ImPanelWidget {
public:
    ImButton();
    virtual ~ImButton() = default;

    // ==================== 内容管理 ====================

    /**
     * @brief 设置按钮内容
     * @param child 子控件（智能指针）
     */
    void SetContent(const Ptr& child);

    /**
     * @brief 获取按钮内容
     * @return 子控件（智能指针）
     */
    Ptr GetContent();

    /**
     * @brief 获取内容 Slot
     * @return ImPaddingSlot 指针
     */
    ImPaddingSlot* GetContentSlot();

    // ==================== 便捷方法（向后兼容） ====================

    /**
     * @brief 设置按钮文本（便捷方法）
     *
     * 内部会自动创建 ImTextBlock 作为内容。
     * 如果已有内容，会被替换。
     *
     * @param text 按钮文本
     */
    void SetText(const std::string& text);

    /**
     * @brief 获取按钮文本（便捷方法）
     *
     * 如果内容是 ImTextBlock，返回其文本；否则返回空字符串。
     *
     * @return 按钮文本
     */
    std::string GetText() const;

    // ==================== 样式设置 ====================

    /**
     * @brief 设置按钮样式
     * @param style 按钮样式集
     */
    void SetStyle(const FButtonStyle& style) { m_Style = style; }

    /**
     * @brief 获取按钮样式
     * @return 按钮样式集
     */
    const FButtonStyle& GetStyle() const { return m_Style; }

    /**
     * @brief 设置正常状态样式
     * @param style 正常状态样式
     */
    void SetNormalStyle(const FButtonStateStyle& style) { m_Style.Normal = style; }

    /**
     * @brief 设置悬停状态样式
     * @param style 悬停状态样式
     */
    void SetHoveredStyle(const FButtonStateStyle& style) { m_Style.Hovered = style; }

    /**
     * @brief 设置按下状态样式
     * @param style 按下状态样式
     */
    void SetPressedStyle(const FButtonStateStyle& style) { m_Style.Pressed = style; }

    /**
     * @brief 设置禁用状态样式
     * @param style 禁用状态样式
     */
    void SetDisabledStyle(const FButtonStateStyle& style) { m_Style.Disabled = style; }

    // ==================== 状态管理 ====================

    /**
     * @brief 设置是否禁用
     * @param bDisabled 是否禁用
     */
    void SetDisabled(bool bDisabled) {
        if (m_bDisabled == bDisabled) {
            return;
        }

        m_bDisabled = bDisabled;
        Invalidate(EInvalidateReason::Paint);
    }

    /**
     * @brief 获取是否禁用
     * @return 是否禁用
     */
    bool IsDisabled() const { return m_bDisabled; }

    /**
     * @brief 获取是否悬停
     * @return 是否悬停
     */
    bool IsHovered() const { return m_bHovered; }

    /**
     * @brief 获取是否按下
     * @return 是否按下
     */
    bool IsPressed() const { return m_bPressed; }

    // ==================== 事件回调 ====================

    /**
     * @brief 设置点击回调
     * @param callback 点击回调函数
     */
    void SetOnClicked(std::function<void()> callback) { m_OnClicked = callback; }

    /**
     * @brief 设置按下回调
     * @param callback 按下回调函数
     */
    void SetOnPressed(std::function<void()> callback) { m_OnPressed = callback; }

    /**
     * @brief 设置释放回调
     * @param callback 释放回调函数
     */
    void SetOnReleased(std::function<void()> callback) { m_OnReleased = callback; }

    /**
     * @brief 设置悬停开始回调
     * @param callback 悬停开始回调函数
     */
    void SetOnHoverBegin(std::function<void()> callback) { m_OnHoverBegin = callback; }

    /**
     * @brief 设置悬停结束回调
     * @param callback 悬停结束回调函数
     */
    void SetOnHoverEnd(std::function<void()> callback) { m_OnHoverEnd = callback; }

    // ==================== 重写基类方法 ====================

    /**
     * @brief 创建 Slot（重写以返回 ImPaddingSlot）
     * @return ImPaddingSlot 指针
     */
    virtual std::unique_ptr<ImSlot> CreateSlot() override;

    /**
     * @brief 绘制按钮
     * @param paintContext 绘制上下文
     */
    virtual void Paint(const FPaintContext& paintContext) override;

    /**
     * @brief 获取最小尺寸
     * @return 最小尺寸
     */
    virtual FVector2 GetMinSize() const override;

    /**
     * @brief 处理输入事件
     * @param event 输入事件
     * @return 事件响应
     */
    virtual FReply OnInputEvent(const FInputEvent& event) override;

    /**
     * @brief 重新布局
     *
     * 设置 Slot 的位置和大小，并应用布局。
     */
    virtual void Relayout();

protected:
    /**
     * @brief 获取当前状态的样式
     * @return 当前状态的样式
     */
    const FButtonStateStyle& GetCurrentStateStyle() const;

    /**
     * @brief 设置按下状态
     * @param bPressed 是否按下
     */
    void SetPressed(bool bPressed);
    void SetHovered(bool bHovered);

    /**
     * @brief 触发点击事件
     */
    void TriggerClick();

    /**
     * @brief 绘制按钮背景和边框
     * @param paintContext 绘制上下文
     */
    void RenderButton(const FPaintContext& paintContext);

private:
    // 样式
    FButtonStyle m_Style;

    // 状态
    bool m_bHovered = false;
    bool m_bPressed = false;
    bool m_bDisabled = false;

    // 事件回调
    std::function<void()> m_OnClicked;
    std::function<void()> m_OnPressed;
    std::function<void()> m_OnReleased;
    std::function<void()> m_OnHoverBegin;
    std::function<void()> m_OnHoverEnd;

    // 原始最小尺寸
    FVector2 m_OriginalMinSize {100.0f, 30.0f};
};

} // namespace ImWidgetV4
