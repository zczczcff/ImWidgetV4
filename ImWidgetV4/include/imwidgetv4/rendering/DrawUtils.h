#pragma once

#include <imgui.h>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 绘制辅助工具类，提供颜色、坐标、文本等辅助函数
 */
class DrawUtils {
public:
    // ==================== 颜色辅助函数 ====================

    /**
     * @brief 从 RGBA 分量创建 ImU32 颜色
     * @param nR 红色分量 (0-255)
     * @param nG 绿色分量 (0-255)
     * @param nB 蓝色分量 (0-255)
     * @param nA 透明度分量 (0-255，默认 255 不透明)
     * @return ImU32 颜色值
     */
    static ImU32 ColorFromRGBA(int nR, int nG, int nB, int nA = 255);

    /**
     * @brief 从浮点 RGBA 分量创建 ImU32 颜色
     * @param fR 红色分量 (0.0-1.0)
     * @param fG 绿色分量 (0.0-1.0)
     * @param fB 蓝色分量 (0.0-1.0)
     * @param fA 透明度分量 (0.0-1.0，默认 1.0 不透明)
     * @return ImU32 颜色值
     */
    static ImU32 ColorFromRGBAf(float fR, float fG, float fB, float fA = 1.0f);

    /**
     * @brief 将 ImU32 颜色转换为 RGBA 分量
     * @param uColor ImU32 颜色值
     * @param nR 输出红色分量 (0-255)
     * @param nG 输出绿色分量 (0-255)
     * @param nB 输出蓝色分量 (0-255)
     * @param nA 输出透明度分量 (0-255)
     */
    static void ColorToRGBA(ImU32 uColor, int& nR, int& nG, int& nB, int& nA);

    /**
     * @brief 将 ImU32 颜色转换为浮点 RGBA 分量
     * @param uColor ImU32 颜色值
     * @param fR 输出红色分量 (0.0-1.0)
     * @param fG 输出绿色分量 (0.0-1.0)
     * @param fB 输出蓝色分量 (0.0-1.0)
     * @param fA 输出透明度分量 (0.0-1.0)
     */
    static void ColorToRGBAf(ImU32 uColor, float& fR, float& fG, float& fB, float& fA);

    /**
     * @brief 修改颜色的 Alpha 通道
     * @param uColor 原始颜色
     * @param nAlpha 新的透明度 (0-255)
     * @return 修改后的颜色
     */
    static ImU32 ColorWithAlpha(ImU32 uColor, int nAlpha);

    /**
     * @brief 修改颜色的 Alpha 通道（浮点版本）
     * @param uColor 原始颜色
     * @param fAlpha 新的透明度 (0.0-1.0)
     * @return 修改后的颜色
     */
    static ImU32 ColorWithAlphaf(ImU32 uColor, float fAlpha);

    /**
     * @brief 混合两个颜色
     * @param uColor1 第一个颜色
     * @param uColor2 第二个颜色
     * @param fT 混合因子 (0.0-1.0)，0.0 返回 color1，1.0 返回 color2
     * @return 混合后的颜色
     */
    static ImU32 ColorLerp(ImU32 uColor1, ImU32 uColor2, float fT);

    // ==================== 坐标辅助函数 ====================

    /**
     * @brief 判断点是否在矩形内
     * @param vPoint 点坐标
     * @param vMin 矩形左上角
     * @param vMax 矩形右下角
     * @return 如果点在矩形内返回 true
     */
    static bool RectContains(const ImVec2& vPoint, const ImVec2& vMin, const ImVec2& vMax);

    /**
     * @brief 判断两个矩形是否相交
     * @param vMin1 第一个矩形左上角
     * @param vMax1 第一个矩形右下角
     * @param vMin2 第二个矩形左上角
     * @param vMax2 第二个矩形右下角
     * @return 如果矩形相交返回 true
     */
    static bool RectIntersects(const ImVec2& vMin1, const ImVec2& vMax1,
                               const ImVec2& vMin2, const ImVec2& vMax2);

    /**
     * @brief 扩展矩形
     * @param vMin 矩形左上角（输入输出）
     * @param vMax 矩形右下角（输入输出）
     * @param fAmount 扩展量（正数扩大，负数缩小）
     */
    static void RectExpand(ImVec2& vMin, ImVec2& vMax, float fAmount);

    /**
     * @brief 获取矩形中心点
     * @param vMin 矩形左上角
     * @param vMax 矩形右下角
     * @return 矩形中心点坐标
     */
    static ImVec2 RectCenter(const ImVec2& vMin, const ImVec2& vMax);

    /**
     * @brief 获取矩形尺寸
     * @param vMin 矩形左上角
     * @param vMax 矩形右下角
     * @return 矩形尺寸
     */
    static ImVec2 RectSize(const ImVec2& vMin, const ImVec2& vMax);

    /**
     * @brief 裁剪矩形到另一个矩形内
     * @param vMin 要裁剪的矩形左上角（输入输出）
     * @param vMax 要裁剪的矩形右下角（输入输出）
     * @param vClipMin 裁剪区域左上角
     * @param vClipMax 裁剪区域右下角
     */
    static void RectClip(ImVec2& vMin, ImVec2& vMax,
                         const ImVec2& vClipMin, const ImVec2& vClipMax);

    // ==================== 向量辅助函数 ====================

    /**
     * @brief 计算两点之间的距离
     * @param vP1 第一个点
     * @param vP2 第二个点
     * @return 距离
     */
    static float Distance(const ImVec2& vP1, const ImVec2& vP2);

    /**
     * @brief 计算两点之间的距离平方（避免开方运算）
     * @param vP1 第一个点
     * @param vP2 第二个点
     * @return 距离平方
     */
    static float DistanceSquared(const ImVec2& vP1, const ImVec2& vP2);

    /**
     * @brief 向量线性插值
     * @param vA 起始向量
     * @param vB 结束向量
     * @param fT 插值因子 (0.0-1.0)
     * @return 插值结果
     */
    static ImVec2 Lerp(const ImVec2& vA, const ImVec2& vB, float fT);

    // ==================== 文本辅助函数 ====================

    /**
     * @brief 计算文本尺寸
     * @param strText 文本内容
     * @param pFont 字体指针（nullptr 使用默认字体）
     * @param fFontSize 字体大小（0.0f 使用默认大小）
     * @return 文本尺寸
     */
    static ImVec2 CalcTextSize(const std::string& strText,
                               ImFont* pFont = nullptr,
                               float fFontSize = 0.0f);

    /**
     * @brief 获取默认字体
     * @return 默认字体指针
     */
    static ImFont* GetDefaultFont();

    /**
     * @brief 计算文本在矩形内的对齐位置
     * @param strText 文本内容
     * @param vMin 矩形左上角
     * @param vMax 矩形右下角
     * @param fAlignX 水平对齐 (0.0=左对齐, 0.5=居中, 1.0=右对齐)
     * @param fAlignY 垂直对齐 (0.0=顶部对齐, 0.5=居中, 1.0=底部对齐)
     * @param pFont 字体指针（nullptr 使用默认字体）
     * @param fFontSize 字体大小（0.0f 使用默认大小）
     * @return 文本绘制位置
     */
    static ImVec2 CalcTextAlignedPos(const std::string& strText,
                                     const ImVec2& vMin, const ImVec2& vMax,
                                     float fAlignX = 0.5f, float fAlignY = 0.5f,
                                     ImFont* pFont = nullptr,
                                     float fFontSize = 0.0f);

    // ==================== 常用颜色常量 ====================

    static const ImU32 COLOR_WHITE;
    static const ImU32 COLOR_BLACK;
    static const ImU32 COLOR_RED;
    static const ImU32 COLOR_GREEN;
    static const ImU32 COLOR_BLUE;
    static const ImU32 COLOR_YELLOW;
    static const ImU32 COLOR_CYAN;
    static const ImU32 COLOR_MAGENTA;
    static const ImU32 COLOR_TRANSPARENT;
};

} // namespace ImWidgetV4
