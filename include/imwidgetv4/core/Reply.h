#pragma once

namespace ImWidgetV4 {

/**
 * @brief 事件响应结构
 *
 * 控件处理输入事件后返回此结构，用于控制事件传播。
 * - Handled: 事件已处理，停止传播
 * - Unhandled: 事件未处理，继续传播
 */
struct FReply {
    bool bHandled;

    FReply() : bHandled(false) {}
    explicit FReply(bool handled) : bHandled(handled) {}

    /**
     * @brief 创建已处理的响应
     *
     * 表示事件已被处理，停止事件传播。
     * @return 已处理的响应
     */
    static FReply Handled() {
        return FReply(true);
    }

    /**
     * @brief 创建未处理的响应
     *
     * 表示事件未被处理，继续事件传播。
     * @return 未处理的响应
     */
    static FReply Unhandled() {
        return FReply(false);
    }

    /**
     * @brief 检查事件是否已处理
     * @return 如果事件已处理返回 true
     */
    bool IsHandled() const {
        return bHandled;
    }
};

} // namespace ImWidgetV4
