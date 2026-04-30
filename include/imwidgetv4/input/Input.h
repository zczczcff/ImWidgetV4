#pragma once
#include <imwidgetv4/core/Types.h>
#include <imgui.h>
#include <cstdint>
#include <vector>

namespace ImWidgetV4 {

// 输入事件类型
enum class EInputEventType {
    None,
    MouseMove,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput
};

// 鼠标按钮
enum class EMouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Extra1 = 3,
    Extra2 = 4
};

// 输入修饰键
struct FInputModifiers {
    bool bCtrl = false;
    bool bShift = false;
    bool bAlt = false;
    bool bSuper = false;

    FInputModifiers() = default;
    FInputModifiers(bool ctrl, bool shift, bool alt, bool super_)
        : bCtrl(ctrl), bShift(shift), bAlt(alt), bSuper(super_) {}

    bool HasAny() const {
        return bCtrl || bShift || bAlt || bSuper;
    }

    bool operator==(const FInputModifiers& other) const {
        return bCtrl == other.bCtrl &&
               bShift == other.bShift &&
               bAlt == other.bAlt &&
               bSuper == other.bSuper;
    }
};

// 输入事件
struct FInputEvent {
    EInputEventType Type = EInputEventType::None;
    FVector2 MousePosition {0.0f, 0.0f};
    FVector2 ScrollDelta {0.0f, 0.0f};
    EMouseButton MouseButton = EMouseButton::Left;
    ImGuiKey Key = ImGuiKey_None;
    unsigned int Codepoint = 0;
    FInputModifiers Modifiers;
    double Timestamp = 0.0;

    FInputEvent() = default;

    // 工具方法
    bool IsMouseEvent() const {
        return Type == EInputEventType::MouseMove ||
               Type == EInputEventType::MouseButtonDown ||
               Type == EInputEventType::MouseButtonUp ||
               Type == EInputEventType::MouseWheel;
    }

    bool IsKeyboardEvent() const {
        return Type == EInputEventType::KeyDown ||
               Type == EInputEventType::KeyUp ||
               Type == EInputEventType::TextInput;
    }

    bool IsMouseButtonEvent() const {
        return Type == EInputEventType::MouseButtonDown ||
               Type == EInputEventType::MouseButtonUp;
    }
};

// ImGui 输入适配器
class FImGuiInputAdapter {
public:
    FImGuiInputAdapter();

    /**
     * @brief 从 ImGui IO 轮询输入事件
     * @param io ImGui IO 对象
     * @param timestamp 当前时间戳
     * @return 输入事件列表
     */
    std::vector<FInputEvent> Poll(const ImGuiIO& io, double timestamp);

private:
    // 上一帧的状态
    FVector2 m_LastMousePosition {0.0f, 0.0f};
    bool m_LastMouseButtons[5] = {false};
    bool m_LastKeys[ImGuiKey_COUNT] = {false};
};

} // namespace ImWidgetV4
