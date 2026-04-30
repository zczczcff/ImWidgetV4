#pragma once
#include <imwidgetv4/core/Types.h>
#include <imwidgetv4/core/Reply.h>
#include <imwidgetv4/input/Input.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

/**
 * @brief Widget 基类
 *
 * 所有 UI 控件的基类，提供基础的控件功能。
 */
class ImWidget : public std::enable_shared_from_this<ImWidget> {
public:
    using Ptr = std::shared_ptr<ImWidget>;

    ImWidget();
    virtual ~ImWidget() = default;

    /**
     * @brief 设置控件名称
     * @param name 控件名称
     */
    void SetName(const std::string& name) { m_Name = name; }

    /**
     * @brief 获取控件名称
     * @return 控件名称
     */
    const std::string& GetName() const { return m_Name; }

    /**
     * @brief 设置可见性
     * @param bVisible 是否可见
     */
    void SetVisible(bool bVisible) { m_bVisible = bVisible; }

    /**
     * @brief 获取可见性
     * @return 是否可见
     */
    bool IsVisible() const { return m_bVisible; }

    /**
     * @brief 设置命中测试可见性
     * @param bVisible 是否参与命中测试
     */
    void SetHitTestVisible(bool bVisible) { m_bHitTestVisible = bVisible; }

    /**
     * @brief 获取命中测试可见性
     * @return 是否参与命中测试
     */
    bool IsHitTestVisible() const { return m_bHitTestVisible; }

    /**
     * @brief 设置是否支持键盘焦点
     * @param bSupports 是否支持键盘焦点
     */
    void SetSupportsKeyboardFocus(bool bSupports) { m_bSupportsKeyboardFocus = bSupports; }

    /**
     * @brief 获取是否支持键盘焦点
     * @return 是否支持键盘焦点
     */
    bool SupportsKeyboardFocus() const { return m_bSupportsKeyboardFocus; }

    /**
     * @brief 设置是否拥有键盘焦点
     * @param bHasFocus 是否拥有焦点
     */
    void SetHasKeyboardFocus(bool bHasFocus) { m_bHasKeyboardFocus = bHasFocus; }

    /**
     * @brief 获取是否拥有键盘焦点
     * @return 是否拥有焦点
     */
    bool HasKeyboardFocus() const { return m_bHasKeyboardFocus; }

    /**
     * @brief 绘制控件
     *
     * 子类应重写此方法以实现自定义绘制逻辑。
     * @param paintContext 绘制上下文，包含 DrawContext、几何信息、样式集等
     */
    virtual void Paint(const FPaintContext& paintContext);

    /**
     * @brief 获取控件的最小尺寸
     *
     * 子类应重写此方法以返回控件所需的最小尺寸。
     * @return 最小尺寸
     */
    virtual FVector2 GetMinSize() const;

    /**
     * @brief 设置控件的几何信息（位置和大小）
     * @param geometry 几何信息
     */
    void SetGeometry(const FGeometry& geometry) { m_Geometry = geometry; }

    /**
     * @brief 获取控件的几何信息
     * @return 几何信息
     */
    const FGeometry& GetGeometry() const { return m_Geometry; }

    // ==================== 事件处理 ====================

    /**
     * @brief 预览输入事件（从根到叶）
     *
     * 在正常事件处理之前调用，允许父控件拦截事件。
     * @param event 输入事件
     * @return 事件响应（Handled 停止传播，Unhandled 继续传播）
     */
    virtual FReply OnPreviewInputEvent(const FInputEvent& event);

    /**
     * @brief 处理输入事件（从叶到根）
     *
     * 在预览阶段之后调用，允许控件处理事件。
     * @param event 输入事件
     * @return 事件响应（Handled 停止传播，Unhandled 继续传播）
     */
    virtual FReply OnInputEvent(const FInputEvent& event);

    /**
     * @brief 构建命中测试路径
     *
     * 从根控件开始，递归查找包含指定位置的控件。
     * @param position 测试位置
     * @param outPath 输出路径（从根到叶）
     * @return 如果命中返回 true
     */
    virtual bool BuildHitTestPath(const FVector2& position, std::vector<Ptr>& outPath);

    /**
     * @brief 添加子控件
     * @param child 子控件
     */
    virtual void AddChild(const Ptr& child);

    /**
     * @brief 移除子控件
     * @param child 子控件
     */
    virtual void RemoveChild(const Ptr& child);

    /**
     * @brief 获取所有子控件
     * @return 子控件列表
     */
    const std::vector<Ptr>& GetChildren() const { return m_Children; }

protected:
    std::string m_Name;                     // 控件名称
    bool m_bVisible = true;                 // 是否可见
    bool m_bHitTestVisible = true;          // 是否参与命中测试
    bool m_bSupportsKeyboardFocus = false;  // 是否支持键盘焦点
    bool m_bHasKeyboardFocus = false;       // 是否拥有键盘焦点
    FGeometry m_Geometry;                   // 几何信息（位置和大小）
    std::vector<Ptr> m_Children;            // 子控件列表
};

} // namespace ImWidgetV4
