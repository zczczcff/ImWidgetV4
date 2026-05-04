#pragma once

#include <imwidgetv4/core/Types.h>
#include <cstdint>
#include <vector>

namespace ImWidgetV4 {

enum class EInputEventType {
    None,
    MouseMove,
    MouseEnter,
    MouseLeave,
    MouseButtonDown,
    MouseButtonUp,
    MouseWheel,
    KeyDown,
    KeyUp,
    TextInput
};

enum class EMouseButton {
    Left = 0,
    Right = 1,
    Middle = 2,
    Extra1 = 3,
    Extra2 = 4
};

enum class EKey {
    None = 0,
    Enter,
    Space,
    Tab,
    Escape,
    Backspace,
    DeleteKey,
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    PageUp,
    PageDown,
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,
    Num0,
    Num1,
    Num2,
    Num3,
    Num4,
    Num5,
    Num6,
    Num7,
    Num8,
    Num9,
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,
    Count
};

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

struct FInputEvent {
    EInputEventType Type = EInputEventType::None;
    FVector2 MousePosition {0.0f, 0.0f};
    FVector2 ScrollDelta {0.0f, 0.0f};
    EMouseButton MouseButton = EMouseButton::Left;
    EKey Key = EKey::None;
    std::int32_t NativeKeyCode = 0;
    std::int32_t ScanCode = 0;
    unsigned int Codepoint = 0;
    FInputModifiers Modifiers;
    double Timestamp = 0.0;

    FInputEvent() = default;

    bool IsMouseEvent() const {
        return Type == EInputEventType::MouseMove ||
               Type == EInputEventType::MouseEnter ||
               Type == EInputEventType::MouseLeave ||
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

class IInputSource {
public:
    virtual ~IInputSource() = default;
    virtual std::vector<FInputEvent> Poll(const FFrameInfo& frameInfo) = 0;
};

} // namespace ImWidgetV4
