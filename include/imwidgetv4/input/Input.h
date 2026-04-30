#pragma once
#include <imwidgetv4/core/Types.h>
#include <cstdint>

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
    Char
};

// 鼠标按钮
enum class EMouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Button4 = 3,
    Button5 = 4
};

// 键盘按键（简化版，使用 ImGui 的键码）
enum class EKey {
    None = 0,
    Tab,
    LeftArrow,
    RightArrow,
    UpArrow,
    DownArrow,
    PageUp,
    PageDown,
    Home,
    End,
    Insert,
    Delete,
    Backspace,
    Space,
    Enter,
    Escape,
    LeftCtrl,
    LeftShift,
    LeftAlt,
    LeftSuper,
    RightCtrl,
    RightShift,
    RightAlt,
    RightSuper,
    Menu,
    // 字母键
    A, B, C, D, E, F, G, H, I, J, K, L, M,
    N, O, P, Q, R, S, T, U, V, W, X, Y, Z,
    // 数字键
    Num0, Num1, Num2, Num3, Num4,
    Num5, Num6, Num7, Num8, Num9,
    // 功能键
    F1, F2, F3, F4, F5, F6,
    F7, F8, F9, F10, F11, F12
};

// 输入事件
struct FInputEvent {
    EInputEventType Type = EInputEventType::None;
    FVector2 Position {0.0f, 0.0f};
    EMouseButton MouseButton = EMouseButton::Left;
    float WheelDelta = 0.0f;
    EKey Key = EKey::None;
    int KeyCode = 0;
    char Character = 0;
    double Timestamp = 0.0;

    // 修饰键
    bool bShiftDown = false;
    bool bCtrlDown = false;
    bool bAltDown = false;
    bool bSuperDown = false;

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
               Type == EInputEventType::Char;
    }

    bool IsMouseButtonEvent() const {
        return Type == EInputEventType::MouseButtonDown ||
               Type == EInputEventType::MouseButtonUp;
    }
};

} // namespace ImWidgetV4
