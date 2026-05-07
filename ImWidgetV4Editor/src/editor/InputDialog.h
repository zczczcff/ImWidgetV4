#pragma once

#include <imwidgetv4/core/Application.h>
#include <functional>
#include <memory>
#include <string>

namespace ImWidgetV4 {
class ImButton;
class ImEditableText;
class ImWidget;
class ImWindow;
}

namespace ImWidgetV4Editor {

struct FInputDialogOptions {
    std::string PopupTitle;
    std::string HeadingText;
    std::string InitialText;
    std::string ConfirmText = "OK";
    std::string CancelText = "Cancel";
    ImWidgetV4::FVector2 Position {220.0f, 120.0f};
    ImWidgetV4::FVector2 Size {360.0f, 116.0f};
    bool bSelectAllOnOpen = true;
    std::function<void(const std::string& text)> OnConfirm;
    std::function<void()> OnCancel;
};

class InputDialog : public std::enable_shared_from_this<InputDialog> {
public:
    bool Open(ImWidgetV4::ImApplication& app, const FInputDialogOptions& options);
    void Close(ImWidgetV4::ImApplication& app);
    bool IsOpen() const;

    std::shared_ptr<ImWidgetV4::ImWindow> GetWindow() const { return m_Window; }
    std::shared_ptr<ImWidgetV4::ImEditableText> GetEditor() const { return m_Editor; }

private:
    void Reset();

    FInputDialogOptions m_Options;
    std::shared_ptr<ImWidgetV4::ImWidget> m_Root;
    std::shared_ptr<ImWidgetV4::ImEditableText> m_Editor;
    std::shared_ptr<ImWidgetV4::ImButton> m_ConfirmButton;
    std::shared_ptr<ImWidgetV4::ImButton> m_CancelButton;
    std::shared_ptr<ImWidgetV4::ImWindow> m_Window;
};

} // namespace ImWidgetV4Editor
