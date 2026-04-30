#include <imwidgetv4/input/Input.h>

namespace ImWidgetV4 {

FImGuiInputAdapter::FImGuiInputAdapter() {
    // 初始化上一帧的状态
    for (int i = 0; i < 5; ++i) {
        m_LastMouseButtons[i] = false;
    }
    for (int i = 0; i < ImGuiKey_COUNT; ++i) {
        m_LastKeys[i] = false;
    }
}

std::vector<FInputEvent> FImGuiInputAdapter::Poll(const ImGuiIO& io, double timestamp) {
    std::vector<FInputEvent> events;

    // 获取修饰键状态
    FInputModifiers modifiers(
        io.KeyCtrl,
        io.KeyShift,
        io.KeyAlt,
        io.KeySuper
    );

    // 1. 鼠标移动事件
    FVector2 currentMousePos(io.MousePos.x, io.MousePos.y);
    if (currentMousePos != m_LastMousePosition) {
        FInputEvent event;
        event.Type = EInputEventType::MouseMove;
        event.MousePosition = currentMousePos;
        event.Modifiers = modifiers;
        event.Timestamp = timestamp;
        events.push_back(event);
        m_LastMousePosition = currentMousePos;
    }

    // 2. 鼠标按钮事件
    const int mouseButtonCount = 5;
    for (int i = 0; i < mouseButtonCount; ++i) {
        bool currentState = io.MouseDown[i];
        bool lastState = m_LastMouseButtons[i];

        if (currentState && !lastState) {
            // 按钮按下
            FInputEvent event;
            event.Type = EInputEventType::MouseButtonDown;
            event.MousePosition = currentMousePos;
            event.MouseButton = static_cast<EMouseButton>(i);
            event.Modifiers = modifiers;
            event.Timestamp = timestamp;
            events.push_back(event);
        } else if (!currentState && lastState) {
            // 按钮释放
            FInputEvent event;
            event.Type = EInputEventType::MouseButtonUp;
            event.MousePosition = currentMousePos;
            event.MouseButton = static_cast<EMouseButton>(i);
            event.Modifiers = modifiers;
            event.Timestamp = timestamp;
            events.push_back(event);
        }

        m_LastMouseButtons[i] = currentState;
    }

    // 3. 鼠标滚轮事件
    if (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f) {
        FInputEvent event;
        event.Type = EInputEventType::MouseWheel;
        event.MousePosition = currentMousePos;
        event.ScrollDelta = FVector2(io.MouseWheelH, io.MouseWheel);
        event.Modifiers = modifiers;
        event.Timestamp = timestamp;
        events.push_back(event);
    }

    // 4. 键盘事件
    for (int i = 0; i < ImGuiKey_COUNT; ++i) {
        ImGuiKey key = static_cast<ImGuiKey>(i);
        bool currentState = ImGui::IsKeyDown(key);
        bool lastState = m_LastKeys[i];

        if (currentState && !lastState) {
            // 按键按下
            FInputEvent event;
            event.Type = EInputEventType::KeyDown;
            event.Key = key;
            event.MousePosition = currentMousePos;
            event.Modifiers = modifiers;
            event.Timestamp = timestamp;
            events.push_back(event);
        } else if (!currentState && lastState) {
            // 按键释放
            FInputEvent event;
            event.Type = EInputEventType::KeyUp;
            event.Key = key;
            event.MousePosition = currentMousePos;
            event.Modifiers = modifiers;
            event.Timestamp = timestamp;
            events.push_back(event);
        }

        m_LastKeys[i] = currentState;
    }

    // 5. 文本输入事件
    for (int i = 0; i < io.InputQueueCharacters.Size; ++i) {
        FInputEvent event;
        event.Type = EInputEventType::TextInput;
        event.Codepoint = io.InputQueueCharacters[i];
        event.MousePosition = currentMousePos;
        event.Modifiers = modifiers;
        event.Timestamp = timestamp;
        events.push_back(event);
    }

    return events;
}

} // namespace ImWidgetV4
