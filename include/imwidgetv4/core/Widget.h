#pragma once
#include <imwidgetv4/core/Types.h>
#include <memory>
#include <string>
#include <vector>

namespace ImWidgetV4 {

/**
 * @brief Widget 基类
 *
 * 所有 UI 控件的基类，提供基础的控件功能。
 */
class ImWidget {
public:
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

protected:
    std::string m_Name;           // 控件名称
    bool m_bVisible = true;       // 是否可见
    FGeometry m_Geometry;         // 几何信息（位置和大小）
};

} // namespace ImWidgetV4
