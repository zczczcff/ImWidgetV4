#pragma once

#include <imwidgetv4/input/Input.h>
#include <array>
#include <cstddef>

struct ImGuiIO;

namespace ImWidgetV4 {

struct FImGuiInputSnapshot {
    FVector2 MousePosition {0.0f, 0.0f};
    bool bHasMousePosition = false;
    std::array<bool, 5> MouseButtons {};
    FVector2 MouseWheelDelta {0.0f, 0.0f};
    std::array<bool, static_cast<std::size_t>(EKey::Count)> Keys {};
    FInputModifiers Modifiers;
    std::vector<unsigned int> TextInput;
};

class FImGuiInputSource : public IInputSource {
public:
    FImGuiInputSource();

    void SetSnapshot(const FImGuiInputSnapshot& snapshot);
    std::vector<FInputEvent> Poll(const FFrameInfo& frameInfo) override;

private:
    FImGuiInputSnapshot PendingSnapshot_;
    FVector2 LastMousePosition_ {0.0f, 0.0f};
    bool bHasLastMousePosition_ = false;
    std::array<bool, 5> LastMouseButtons_ {};
    std::array<bool, static_cast<std::size_t>(EKey::Count)> LastKeys_ {};
};

void PopulateImGuiInputSnapshotFromIo(const ImGuiIO& io, FImGuiInputSnapshot& snapshot);

} // namespace ImWidgetV4
