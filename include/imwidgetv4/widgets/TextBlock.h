#pragma once
#include <imwidgetv4/core/Widget.h>
#include <imwidgetv4/core/Types.h>
#include <string>

namespace ImWidgetV4 {

/**
 * @brief 文本对齐方式（水平）
 */
enum class ETextAlignment {
    Left,       // 左对齐
    Center,     // 居中对齐
    Right       // 右对齐
};

/**
 * @brief 文本对齐方式（垂直）
 */
enum class EVerticalAlignment {
    Top,        // 顶部对齐
    Center,     // 居中对齐
    Bottom      // 底部对齐
};

/**
 * @brief 文本显示控件
 *
 * ImTextBlock 是一个基础的文本显示控件，支持：
 * - 文本内容设置
 * - 文本颜色自定义
 * - 字体大小调整
 * - 水平和垂直对齐
 * - 自动换行（可选）
 */
class ImTextBlock : public ImWidget {
public:
    ImTextBlock();
    virtual ~ImTextBlock() = default;

    /**
     * @brief 设置文本内容
     * @param text 文本内容
     */
    void SetText(const std::string& text);

    /**
     * @brief 获取文本内容
     * @return 文本内容
     */
    const std::string& GetText() const { return m_Text; }

    /**
     * @brief 设置文本颜色
     * @param color 文本颜色
     */
    void SetTextColor(const FColor& color);

    /**
     * @brief 获取文本颜色
     * @return 文本颜色
     */
    const FColor& GetTextColor() const { return m_TextColor; }

    /**
     * @brief 设置字体大小
     * @param size 字体大小（像素）
     */
    void SetFontSize(float size);

    /**
     * @brief 获取字体大小
     * @return 字体大小
     */
    float GetFontSize() const { return m_FontSize; }

    /**
     * @brief 设置文本水平对齐方式
     * @param alignment 对齐方式
     */
    void SetTextAlignment(ETextAlignment alignment);

    /**
     * @brief 获取文本水平对齐方式
     * @return 对齐方式
     */
    ETextAlignment GetTextAlignment() const { return m_TextAlignment; }

    /**
     * @brief 设置文本垂直对齐方式
     * @param alignment 对齐方式
     */
    void SetVerticalAlignment(EVerticalAlignment alignment);

    /**
     * @brief 获取文本垂直对齐方式
     * @return 对齐方式
     */
    EVerticalAlignment GetVerticalAlignment() const { return m_VerticalAlignment; }

    /**
     * @brief 设置是否自动换行
     * @param bWrap 是否自动换行
     */
    void SetWrapText(bool bWrap);

    /**
     * @brief 获取是否自动换行
     * @return 是否自动换行
     */
    bool GetWrapText() const { return m_bWrapText; }

    /**
     * @brief 渲染控件
     *
     * 重写基类方法，使用 ImGui 绘制文本。
     */
    void Render() override;

    /**
     * @brief 获取控件的最小尺寸
     *
     * 重写基类方法，返回文本所需的最小尺寸。
     * @return 最小尺寸
     */
    FVector2 GetMinSize() const override;

private:
    std::string m_Text;                             // 文本内容
    FColor m_TextColor;                             // 文本颜色
    float m_FontSize;                               // 字体大小
    ETextAlignment m_TextAlignment;                 // 水平对齐方式
    EVerticalAlignment m_VerticalAlignment;         // 垂直对齐方式
    bool m_bWrapText;                               // 是否自动换行

    /**
     * @brief 计算文本尺寸
     *
     * 使用 ImGui::CalcTextSize 计算文本的实际尺寸。
     * @return 文本尺寸
     */
    FVector2 CalculateTextSize() const;

    /**
     * @brief 计算文本位置
     *
     * 根据对齐方式和控件几何信息计算文本的绘制位置。
     * @param textSize 文本尺寸
     * @return 文本位置
     */
    FVector2 CalculateTextPosition(const FVector2& textSize) const;
};

} // namespace ImWidgetV4
