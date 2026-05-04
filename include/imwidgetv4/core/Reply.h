#pragma once

#include <imwidgetv4/input/Input.h>
#include <memory>

namespace ImWidgetV4 {

class ImWidget;

struct FReply {
    bool bHandled = false;
    bool bReleaseMouseCapture = false;
    bool bClearKeyboardFocus = false;
    std::shared_ptr<ImWidget> MouseCaptureTarget;
    std::shared_ptr<ImWidget> FocusTarget;
    EMouseButton MouseCaptureButton = EMouseButton::Left;

    FReply() = default;
    explicit FReply(bool handled) : bHandled(handled) {}

    static FReply Handled() {
        return FReply(true);
    }

    static FReply Unhandled() {
        return FReply(false);
    }

    bool IsHandled() const {
        return bHandled;
    }

    FReply& CaptureMouse(const std::shared_ptr<ImWidget>& widget, EMouseButton button) {
        MouseCaptureTarget = widget;
        MouseCaptureButton = button;
        bReleaseMouseCapture = false;
        return *this;
    }

    FReply& ReleaseMouseCapture() {
        MouseCaptureTarget.reset();
        bReleaseMouseCapture = true;
        return *this;
    }

    FReply& SetKeyboardFocus(const std::shared_ptr<ImWidget>& widget) {
        FocusTarget = widget;
        bClearKeyboardFocus = false;
        return *this;
    }

    FReply& ClearKeyboardFocus() {
        FocusTarget.reset();
        bClearKeyboardFocus = true;
        return *this;
    }
};

} // namespace ImWidgetV4
