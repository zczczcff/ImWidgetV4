#pragma once
#include <imwidgetv4/core/Types.h>
#include <imgui.h>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 绘制上下文类
 *
 * 封装 ImGui 的 ImDrawList 绘制调用，提供类型安全的绘制接口。
 * 控件通过此类进行绘制，而不是直接调用 ImGui API。
 */
class DrawContext {
public:
    /**
     * @brief 构造函数
     * @param drawList ImGui 的绘制列表指针
     */
    explicit DrawContext(ImDrawList* drawList);

    // ==================== 文本绘制 ====================

    /**
     * @brief 绘制文本
     * @param position 文本位置
     * @param color 文本颜色
     * @param text 文本内容
     * @param fontSize 字体大小（0 表示使用默认字体大小）
     */
    void DrawText(const FVector2& position, const FColor& color,
                  const std::string& text, float fontSize = 0.0f);

    // ==================== 矩形绘制 ====================

    /**
     * @brief 绘制矩形边框
     * @param min 左上角位置
     * @param max 右下角位置
     * @param color 边框颜色
     * @param rounding 圆角半径
     * @param thickness 边框粗细
     */
    void DrawRect(const FVector2& min, const FVector2& max,
                  const FColor& color, float rounding = 0.0f,
                  float thickness = 1.0f);

    /**
     * @brief 绘制填充矩形
     * @param min 左上角位置
     * @param max 右下角位置
     * @param color 填充颜色
     * @param rounding 圆角半径
     */
    void DrawRectFilled(const FVector2& min, const FVector2& max,
                        const FColor& color, float rounding = 0.0f);

    // ==================== 线条绘制 ====================

    /**
     * @brief 绘制线条
     * @param p1 起点
     * @param p2 终点
     * @param color 线条颜色
     * @param thickness 线条粗细
     */
    void DrawLine(const FVector2& p1, const FVector2& p2,
                  const FColor& color, float thickness = 1.0f);

    // ==================== 圆形绘制 ====================

    /**
     * @brief 绘制圆形边框
     * @param center 圆心位置
     * @param radius 半径
     * @param color 边框颜色
     * @param numSegments 分段数（0 表示自动计算）
     * @param thickness 边框粗细
     */
    void DrawCircle(const FVector2& center, float radius,
                    const FColor& color, int numSegments = 0,
                    float thickness = 1.0f);

    /**
     * @brief 绘制填充圆形
     * @param center 圆心位置
     * @param radius 半径
     * @param color 填充颜色
     * @param numSegments 分段数（0 表示自动计算）
     */
    void DrawCircleFilled(const FVector2& center, float radius,
                          const FColor& color, int numSegments = 0);

    // ==================== 图片绘制 ====================

    /**
     * @brief 绘制图片
     * @param textureId 纹理 ID
     * @param min 左上角位置
     * @param max 右下角位置
     * @param uvMin UV 坐标左上角（默认 (0,0)）
     * @param uvMax UV 坐标右下角（默认 (1,1)）
     * @param tintColor 着色颜色（默认白色）
     */
    void DrawImage(ImTextureID textureId, const FVector2& min,
                   const FVector2& max, const FVector2& uvMin = FVector2(0.0f, 0.0f),
                   const FVector2& uvMax = FVector2(1.0f, 1.0f),
                   const FColor& tintColor = FColor::White);

    // ==================== 路径绘制 ====================

    /**
     * @brief 添加路径点
     * @param pos 路径点位置
     */
    void PathLineTo(const FVector2& pos);

    /**
     * @brief 添加圆弧路径
     * @param center 圆心位置
     * @param radius 半径
     * @param aMin 起始角度（弧度）
     * @param aMax 结束角度（弧度）
     * @param numSegments 分段数（0 表示自动计算）
     */
    void PathArcTo(const FVector2& center, float radius,
                   float aMin, float aMax, int numSegments = 0);

    /**
     * @brief 描边路径
     * @param color 描边颜色
     * @param closed 是否闭合路径
     * @param thickness 描边粗细
     */
    void PathStroke(const FColor& color, bool closed = false,
                    float thickness = 1.0f);

    /**
     * @brief 填充路径
     * @param color 填充颜色
     */
    void PathFill(const FColor& color);

    // ==================== 裁剪区域 ====================

    /**
     * @brief 压入裁剪矩形
     * @param min 左上角位置
     * @param max 右下角位置
     * @param intersectWithCurrent 是否与当前裁剪区域求交
     */
    void PushClipRect(const FVector2& min, const FVector2& max,
                      bool intersectWithCurrent = false);

    /**
     * @brief 弹出裁剪矩形
     */
    void PopClipRect();

    // ==================== 原始访问 ====================

    /**
     * @brief 获取原始 ImDrawList 指针
     *
     * 仅在必要时使用，用于访问未封装的高级功能。
     * @return ImDrawList 指针
     */
    ImDrawList* GetImDrawList() const { return m_DrawList; }

private:
    ImDrawList* m_DrawList;  // ImGui 绘制列表指针
};

} // namespace ImWidgetV4
