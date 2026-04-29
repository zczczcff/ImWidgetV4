#pragma once

#include <imgui.h>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 绘制上下文类，封装 ImDrawList 指针，提供便捷的绘制方法
 *
 * DrawContext 是对 ImGui DrawList API 的轻量级封装，提供更友好的绘制接口。
 * 它不隐藏底层 API，而是提供便捷的辅助方法。
 */
class DrawContext {
public:
    /**
     * @brief 构造函数
     * @param pDrawList ImGui 的 DrawList 指针
     */
    explicit DrawContext(ImDrawList* pDrawList);

    /**
     * @brief 获取底层的 ImDrawList 指针
     * @return ImDrawList 指针
     */
    ImDrawList* GetDrawList() const { return m_pDrawList; }

    // ==================== 矩形绘制 ====================

    /**
     * @brief 绘制填充矩形
     * @param vMin 矩形左上角坐标
     * @param vMax 矩形右下角坐标
     * @param uColor 填充颜色 (RGBA)
     * @param fRounding 圆角半径 (默认 0.0f)
     * @param nFlags 圆角标志 (默认 0，表示所有角)
     */
    void DrawRectFilled(const ImVec2& vMin, const ImVec2& vMax, ImU32 uColor,
                        float fRounding = 0.0f, ImDrawFlags nFlags = 0);

    /**
     * @brief 绘制矩形边框
     * @param vMin 矩形左上角坐标
     * @param vMax 矩形右下角坐标
     * @param uColor 边框颜色 (RGBA)
     * @param fRounding 圆角半径 (默认 0.0f)
     * @param nFlags 圆角标志 (默认 0，表示所有角)
     * @param fThickness 边框宽度 (默认 1.0f)
     */
    void DrawRect(const ImVec2& vMin, const ImVec2& vMax, ImU32 uColor,
                  float fRounding = 0.0f, ImDrawFlags nFlags = 0, float fThickness = 1.0f);

    /**
     * @brief 绘制矩形（同时支持填充和边框）
     * @param vMin 矩形左上角坐标
     * @param vMax 矩形右下角坐标
     * @param uFillColor 填充颜色 (RGBA)，如果为 0 则不填充
     * @param uBorderColor 边框颜色 (RGBA)，如果为 0 则不绘制边框
     * @param fRounding 圆角半径 (默认 0.0f)
     * @param nFlags 圆角标志 (默认 0，表示所有角)
     * @param fBorderThickness 边框宽度 (默认 1.0f)
     */
    void DrawRectangle(const ImVec2& vMin, const ImVec2& vMax,
                       ImU32 uFillColor, ImU32 uBorderColor = 0,
                       float fRounding = 0.0f, ImDrawFlags nFlags = 0,
                       float fBorderThickness = 1.0f);

    // ==================== 圆形绘制 ====================

    /**
     * @brief 绘制填充圆形
     * @param vCenter 圆心坐标
     * @param fRadius 半径
     * @param uColor 填充颜色 (RGBA)
     * @param nNumSegments 分段数 (默认 0，自动计算)
     */
    void DrawCircleFilled(const ImVec2& vCenter, float fRadius, ImU32 uColor,
                          int nNumSegments = 0);

    /**
     * @brief 绘制圆形边框
     * @param vCenter 圆心坐标
     * @param fRadius 半径
     * @param uColor 边框颜色 (RGBA)
     * @param nNumSegments 分段数 (默认 0，自动计算)
     * @param fThickness 边框宽度 (默认 1.0f)
     */
    void DrawCircle(const ImVec2& vCenter, float fRadius, ImU32 uColor,
                    int nNumSegments = 0, float fThickness = 1.0f);

    // ==================== 三角形绘制 ====================

    /**
     * @brief 绘制填充三角形
     * @param vP1 第一个顶点
     * @param vP2 第二个顶点
     * @param vP3 第三个顶点
     * @param uColor 填充颜色 (RGBA)
     */
    void DrawTriangleFilled(const ImVec2& vP1, const ImVec2& vP2, const ImVec2& vP3,
                            ImU32 uColor);

    /**
     * @brief 绘制三角形边框
     * @param vP1 第一个顶点
     * @param vP2 第二个顶点
     * @param vP3 第三个顶点
     * @param uColor 边框颜色 (RGBA)
     * @param fThickness 边框宽度 (默认 1.0f)
     */
    void DrawTriangle(const ImVec2& vP1, const ImVec2& vP2, const ImVec2& vP3,
                      ImU32 uColor, float fThickness = 1.0f);

    // ==================== 线条绘制 ====================

    /**
     * @brief 绘制线条
     * @param vP1 起点
     * @param vP2 终点
     * @param uColor 线条颜色 (RGBA)
     * @param fThickness 线条宽度 (默认 1.0f)
     */
    void DrawLine(const ImVec2& vP1, const ImVec2& vP2, ImU32 uColor,
                  float fThickness = 1.0f);

    // ==================== 文本绘制 ====================

    /**
     * @brief 绘制文本
     * @param vPos 文本位置
     * @param uColor 文本颜色 (RGBA)
     * @param strText 文本内容
     * @param pFont 字体指针 (默认 nullptr，使用默认字体)
     */
    void DrawText(const ImVec2& vPos, ImU32 uColor, const std::string& strText,
                  ImFont* pFont = nullptr);

    /**
     * @brief 绘制文本（带字体大小）
     * @param pFont 字体指针
     * @param fFontSize 字体大小
     * @param vPos 文本位置
     * @param uColor 文本颜色 (RGBA)
     * @param strText 文本内容
     */
    void DrawText(ImFont* pFont, float fFontSize, const ImVec2& vPos,
                  ImU32 uColor, const std::string& strText);

    // ==================== 图像绘制 ====================

    /**
     * @brief 绘制图像
     * @param pTextureId 纹理 ID
     * @param vMin 图像左上角坐标
     * @param vMax 图像右下角坐标
     * @param vUv0 UV 坐标左上角 (默认 (0,0))
     * @param vUv1 UV 坐标右下角 (默认 (1,1))
     * @param uTintColor 着色颜色 (默认白色不透明)
     */
    void DrawImage(ImTextureID pTextureId, const ImVec2& vMin, const ImVec2& vMax,
                   const ImVec2& vUv0 = ImVec2(0, 0), const ImVec2& vUv1 = ImVec2(1, 1),
                   ImU32 uTintColor = IM_COL32_WHITE);

    // ==================== 裁剪区域管理 ====================

    /**
     * @brief 设置裁剪区域
     * @param vMin 裁剪区域左上角
     * @param vMax 裁剪区域右下角
     * @param bIntersectWithCurrentClip 是否与当前裁剪区域求交 (默认 true)
     */
    void PushClipRect(const ImVec2& vMin, const ImVec2& vMax,
                      bool bIntersectWithCurrentClip = true);

    /**
     * @brief 恢复裁剪区域
     */
    void PopClipRect();

private:
    ImDrawList* m_pDrawList; ///< ImGui 的 DrawList 指针
};

} // namespace ImWidgetV4
