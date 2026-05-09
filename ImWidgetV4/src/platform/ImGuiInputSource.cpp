#include <imwidgetv4/platform/ImGuiInputSource.h>
#include <imgui.h>
#include <array>
#include <utility>

namespace ImWidgetV4 {

namespace {

constexpr std::pair<ImGuiKey, EKey> GImGuiKeyMap[] = {
    {ImGuiKey_Enter, EKey::Enter},
    {ImGuiKey_Space, EKey::Space},
    {ImGuiKey_Tab, EKey::Tab},
    {ImGuiKey_Escape, EKey::Escape},
    {ImGuiKey_Backspace, EKey::Backspace},
    {ImGuiKey_Delete, EKey::DeleteKey},
    {ImGuiKey_LeftArrow, EKey::Left},
    {ImGuiKey_RightArrow, EKey::Right},
    {ImGuiKey_UpArrow, EKey::Up},
    {ImGuiKey_DownArrow, EKey::Down},
    {ImGuiKey_Home, EKey::Home},
    {ImGuiKey_End, EKey::End},
    {ImGuiKey_PageUp, EKey::PageUp},
    {ImGuiKey_PageDown, EKey::PageDown},
    {ImGuiKey_A, EKey::A},
    {ImGuiKey_B, EKey::B},
    {ImGuiKey_C, EKey::C},
    {ImGuiKey_D, EKey::D},
    {ImGuiKey_E, EKey::E},
    {ImGuiKey_F, EKey::F},
    {ImGuiKey_G, EKey::G},
    {ImGuiKey_H, EKey::H},
    {ImGuiKey_I, EKey::I},
    {ImGuiKey_J, EKey::J},
    {ImGuiKey_K, EKey::K},
    {ImGuiKey_L, EKey::L},
    {ImGuiKey_M, EKey::M},
    {ImGuiKey_N, EKey::N},
    {ImGuiKey_O, EKey::O},
    {ImGuiKey_P, EKey::P},
    {ImGuiKey_Q, EKey::Q},
    {ImGuiKey_R, EKey::R},
    {ImGuiKey_S, EKey::S},
    {ImGuiKey_T, EKey::T},
    {ImGuiKey_U, EKey::U},
    {ImGuiKey_V, EKey::V},
    {ImGuiKey_W, EKey::W},
    {ImGuiKey_X, EKey::X},
    {ImGuiKey_Y, EKey::Y},
    {ImGuiKey_Z, EKey::Z},
    {ImGuiKey_0, EKey::Num0},
    {ImGuiKey_1, EKey::Num1},
    {ImGuiKey_2, EKey::Num2},
    {ImGuiKey_3, EKey::Num3},
    {ImGuiKey_4, EKey::Num4},
    {ImGuiKey_5, EKey::Num5},
    {ImGuiKey_6, EKey::Num6},
    {ImGuiKey_7, EKey::Num7},
    {ImGuiKey_8, EKey::Num8},
    {ImGuiKey_9, EKey::Num9},
    {ImGuiKey_F1, EKey::F1},
    {ImGuiKey_F2, EKey::F2},
    {ImGuiKey_F3, EKey::F3},
    {ImGuiKey_F4, EKey::F4},
    {ImGuiKey_F5, EKey::F5},
    {ImGuiKey_F6, EKey::F6},
    {ImGuiKey_F7, EKey::F7},
    {ImGuiKey_F8, EKey::F8},
    {ImGuiKey_F9, EKey::F9},
    {ImGuiKey_F10, EKey::F10},
    {ImGuiKey_F11, EKey::F11},
    {ImGuiKey_F12, EKey::F12}
};

constexpr std::size_t ToIndex(EKey key) {
    return static_cast<std::size_t>(key);
}

} // namespace

FImGuiInputSource::FImGuiInputSource() = default;

void FImGuiInputSource::SetSnapshot(const FImGuiInputSnapshot& snapshot) {
    PendingSnapshot_ = snapshot;
}

void FImGuiInputSource::ResetMouseButtonState(EMouseButton button, bool bDown)
{
    const std::size_t index = static_cast<std::size_t>(button);
    if (index >= LastMouseButtons_.size()) {
        return;
    }

    LastMouseButtons_[index] = bDown;
    PendingSnapshot_.MouseButtons[index] = bDown;
}

std::vector<FInputEvent> FImGuiInputSource::Poll(const FFrameInfo& frameInfo) {
    std::vector<FInputEvent> events;

    if (PendingSnapshot_.bHasMousePosition &&
        (!bHasLastMousePosition_ || PendingSnapshot_.MousePosition != LastMousePosition_)) {
        FInputEvent event;
        event.Type = EInputEventType::MouseMove;
        event.MousePosition = PendingSnapshot_.MousePosition;
        event.Modifiers = PendingSnapshot_.Modifiers;
        event.Timestamp = frameInfo.CurrentTime;
        events.push_back(event);
        LastMousePosition_ = PendingSnapshot_.MousePosition;
        bHasLastMousePosition_ = true;
    }

    for (std::size_t i = 0; i < PendingSnapshot_.MouseButtons.size(); ++i) {
        const bool currentState = PendingSnapshot_.MouseButtons[i];
        const bool lastState = LastMouseButtons_[i];
        if (currentState == lastState) {
            continue;
        }

        FInputEvent event;
        event.Type = currentState ? EInputEventType::MouseButtonDown : EInputEventType::MouseButtonUp;
        event.MousePosition = PendingSnapshot_.MousePosition;
        event.MouseButton = static_cast<EMouseButton>(i);
        event.Modifiers = PendingSnapshot_.Modifiers;
        event.Timestamp = frameInfo.CurrentTime;
        events.push_back(event);
        LastMouseButtons_[i] = currentState;
    }

    if (PendingSnapshot_.MouseWheelDelta.X != 0.0f || PendingSnapshot_.MouseWheelDelta.Y != 0.0f) {
        FInputEvent event;
        event.Type = EInputEventType::MouseWheel;
        event.MousePosition = PendingSnapshot_.MousePosition;
        event.ScrollDelta = PendingSnapshot_.MouseWheelDelta;
        event.Modifiers = PendingSnapshot_.Modifiers;
        event.Timestamp = frameInfo.CurrentTime;
        events.push_back(event);
    }

    for (const auto& [imguiKey, key] : GImGuiKeyMap) {
        const std::size_t keyIndex = ToIndex(key);
        const bool currentState = PendingSnapshot_.Keys[keyIndex];
        const bool lastState = LastKeys_[keyIndex];
        if (currentState == lastState) {
            continue;
        }

        FInputEvent event;
        event.Type = currentState ? EInputEventType::KeyDown : EInputEventType::KeyUp;
        event.Key = key;
        event.NativeKeyCode = static_cast<std::int32_t>(imguiKey);
        event.MousePosition = PendingSnapshot_.MousePosition;
        event.Modifiers = PendingSnapshot_.Modifiers;
        event.Timestamp = frameInfo.CurrentTime;
        events.push_back(event);
        LastKeys_[keyIndex] = currentState;
    }

    for (unsigned int codepoint : PendingSnapshot_.TextInput) {
        FInputEvent event;
        event.Type = EInputEventType::TextInput;
        event.Codepoint = codepoint;
        event.MousePosition = PendingSnapshot_.MousePosition;
        event.Modifiers = PendingSnapshot_.Modifiers;
        event.Timestamp = frameInfo.CurrentTime;
        events.push_back(event);
    }

    return events;
}

void PopulateImGuiInputSnapshotFromIo(const ImGuiIO& io, FImGuiInputSnapshot& snapshot) {
    snapshot.MousePosition = FVector2(io.MousePos.x, io.MousePos.y);
    snapshot.bHasMousePosition = io.MousePos.x >= 0.0f && io.MousePos.y >= 0.0f;

    for (std::size_t i = 0; i < snapshot.MouseButtons.size(); ++i) {
        snapshot.MouseButtons[i] = io.MouseDown[i];
    }

    snapshot.MouseWheelDelta = FVector2(io.MouseWheelH, io.MouseWheel);
    snapshot.Modifiers = FInputModifiers(io.KeyCtrl, io.KeyShift, io.KeyAlt, io.KeySuper);

    snapshot.Keys.fill(false);
    for (const auto& [imguiKey, key] : GImGuiKeyMap) {
        snapshot.Keys[ToIndex(key)] = io.KeysData[imguiKey].Down;
    }

    snapshot.TextInput.assign(
        io.InputQueueCharacters.begin(),
        io.InputQueueCharacters.end()
    );
}

} // namespace ImWidgetV4
