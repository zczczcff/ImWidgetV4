#pragma once
#include <imwidgetv4/core/Types.h>

namespace ImWidgetV4 {

/**
 * @brief 按钮状态样式
 *
 * 定义按钮在不同状态下的外观样式。
 */
struct FButtonStateStyle {
    FColor BackgroundColor = FColor::White;  // 背景颜色
    FColor BorderColor = FColor::Black;      // 边框颜色
    FColor TextColor = FColor::Black;        // 文本颜色
    float BorderThickness = 1.0f;            // 边框粗细
    float CornerRadius = 0.0f;               // 圆角半径
    bool bHasBorder = false;                 // 是否显示边框

    FButtonStateStyle() = default;

    FButtonStateStyle(const FColor& bgColor, const FColor& borderColor,
                     const FColor& textColor, float borderThickness = 1.0f,
                     float cornerRadius = 0.0f, bool hasBorder = false)
        : BackgroundColor(bgColor)
        , BorderColor(borderColor)
        , TextColor(textColor)
        , BorderThickness(borderThickness)
        , CornerRadius(cornerRadius)
        , bHasBorder(hasBorder)
    {}
};

/**
 * @brief 按钮样式集
 *
 * 包含按钮所有状态的样式定义。
 */
struct FButtonStyle {
    FButtonStateStyle Normal;    // 正常状态
    FButtonStateStyle Hovered;   // 悬停状态
    FButtonStateStyle Pressed;   // 按下状态
    FButtonStateStyle Focused;   // 焦点状态
    FButtonStateStyle Disabled;  // 禁用状态

    FButtonStyle() {
        // 默认样式 - 浅色主题
        Normal = FButtonStateStyle(
            FColor::FromBytes(245, 250, 255, 255),  // 背景：浅蓝白色
            FColor::FromBytes(200, 220, 240, 255),  // 边框：浅蓝色
            FColor::FromBytes(50, 50, 50, 255),     // 文本：深灰色
            1.0f, 0.0f, false
        );

        Hovered = FButtonStateStyle(
            FColor::FromBytes(210, 230, 250, 255),  // 背景：浅蓝色
            FColor::FromBytes(120, 170, 220, 255),  // 边框：中蓝色
            FColor::FromBytes(30, 30, 30, 255),     // 文本：更深灰色
            1.0f, 0.0f, true
        );

        Pressed = FButtonStateStyle(
            FColor::FromBytes(190, 220, 245, 255),  // 背景：更深蓝色
            FColor::FromBytes(100, 150, 210, 255),  // 边框：深蓝色
            FColor::FromBytes(20, 20, 20, 255),     // 文本：黑色
            2.0f, 0.0f, true
        );

        Focused = FButtonStateStyle(
            FColor::FromBytes(235, 245, 255, 255),  // 背景：浅蓝白色
            FColor::FromBytes(0, 120, 215, 255),    // 边框：蓝色（焦点指示）
            FColor::FromBytes(30, 30, 30, 255),     // 文本：深灰色
            2.0f, 0.0f, true
        );

        Disabled = FButtonStateStyle(
            FColor::FromBytes(240, 240, 240, 255),  // 背景：灰色
            FColor::FromBytes(200, 200, 200, 255),  // 边框：浅灰色
            FColor::FromBytes(150, 150, 150, 255),  // 文本：中灰色
            1.0f, 0.0f, false
        );
    }

    /**
     * @brief 创建主要按钮样式（Primary Button）
     */
    static FButtonStyle CreatePrimary() {
        FButtonStyle style;
        style.Normal = FButtonStateStyle(
            FColor::FromBytes(0, 120, 215, 255),    // 背景：蓝色
            FColor::FromBytes(0, 100, 195, 255),    // 边框：深蓝色
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            1.0f, 4.0f, false
        );
        style.Hovered = FButtonStateStyle(
            FColor::FromBytes(0, 140, 235, 255),    // 背景：亮蓝色
            FColor::FromBytes(0, 120, 215, 255),    // 边框：蓝色
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            1.0f, 4.0f, false
        );
        style.Pressed = FButtonStateStyle(
            FColor::FromBytes(0, 100, 195, 255),    // 背景：深蓝色
            FColor::FromBytes(0, 80, 175, 255),     // 边框：更深蓝色
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            2.0f, 4.0f, true
        );
        style.Focused = FButtonStateStyle(
            FColor::FromBytes(0, 130, 225, 255),    // 背景：中蓝色
            FColor::FromBytes(255, 255, 255, 255),  // 边框：白色（焦点指示）
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            2.0f, 4.0f, true
        );
        style.Disabled = FButtonStateStyle(
            FColor::FromBytes(200, 200, 200, 255),  // 背景：灰色
            FColor::FromBytes(180, 180, 180, 255),  // 边框：浅灰色
            FColor::FromBytes(150, 150, 150, 255),  // 文本：中灰色
            1.0f, 4.0f, false
        );
        return style;
    }

    /**
     * @brief 创建危险按钮样式（Danger Button）
     */
    static FButtonStyle CreateDanger() {
        FButtonStyle style;
        style.Normal = FButtonStateStyle(
            FColor::FromBytes(220, 53, 69, 255),    // 背景：红色
            FColor::FromBytes(200, 33, 49, 255),    // 边框：深红色
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            1.0f, 4.0f, false
        );
        style.Hovered = FButtonStateStyle(
            FColor::FromBytes(240, 73, 89, 255),    // 背景：亮红色
            FColor::FromBytes(220, 53, 69, 255),    // 边框：红色
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            1.0f, 4.0f, false
        );
        style.Pressed = FButtonStateStyle(
            FColor::FromBytes(200, 33, 49, 255),    // 背景：深红色
            FColor::FromBytes(180, 13, 29, 255),    // 边框：更深红色
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            2.0f, 4.0f, true
        );
        style.Focused = FButtonStateStyle(
            FColor::FromBytes(230, 63, 79, 255),    // 背景：中红色
            FColor::FromBytes(255, 255, 255, 255),  // 边框：白色（焦点指示）
            FColor::FromBytes(255, 255, 255, 255),  // 文本：白色
            2.0f, 4.0f, true
        );
        style.Disabled = FButtonStateStyle(
            FColor::FromBytes(200, 200, 200, 255),  // 背景：灰色
            FColor::FromBytes(180, 180, 180, 255),  // 边框：浅灰色
            FColor::FromBytes(150, 150, 150, 255),  // 文本：中灰色
            1.0f, 4.0f, false
        );
        return style;
    }
};

} // namespace ImWidgetV4
