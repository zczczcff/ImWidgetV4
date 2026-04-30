#pragma once
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/widgets/ButtonStyle.h>
#include <functional>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 按钮控件
 *
 * 可点击的按钮控件，支持多种状态和样式。
 * 参考 ImWidget 项目的按钮实现设计。
 */
class ImButton : public ImWidget {
public:
    ImButton();
    virtual ~ImButton() = default;

    // ==================== 文本和内容 ====================

    /**
     * @brief 设置按钮文本
     * @param text 按钮文本
     */
    void SetText(const std::string& text) { m_Text = text; }

    /**
     * @brief 获取按钮文本
     * @return 按钮文本
     */
    const std::string& GetText() const { return m_Text; }

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
    void SetDisabled(bool bDisabled) { m_bDisabled = bDisabled; }

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

protected:
    /**
     * @brief 获取当前状态的样式
     * @return 当前状态的样式
     */
    const FButtonStateStyle& GetCurrentStateStyle() const;

    /**
     * @brief 触发点击事件
     */
    void TriggerClick();

private:
    // 文本和内容
    std::string m_Text;

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

    // 最小尺寸
    FVector2 m_MinSize {100.0f, 30.0f};
};

} // namespace ImWidgetV4
