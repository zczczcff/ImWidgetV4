#pragma once

#include "DocumentFormat.h"

#include <imwidgetv4/core/Widget.h>
#include <memory>
#include <string>

namespace ImWidgetV4Editor {

struct FWidgetSerializationResult {
    bool bSuccess = false;
    std::string ErrorMessage;
    std::shared_ptr<ImWidgetV4::ImWidget> Widget;
};

struct FWidgetSerializationOptions {
    bool bPersistent = false;
};

class WidgetSerializer {
public:
    static json SerializeWidgetTree(const std::shared_ptr<ImWidgetV4::ImWidget>& widget);
    static json SerializeWidgetTree(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const FWidgetSerializationOptions& options);
    static FWidgetSerializationResult DeserializeWidgetTree(const json& widgetJson);

private:
    static json SerializeWidgetNode(
        const std::shared_ptr<ImWidgetV4::ImWidget>& widget,
        const FWidgetSerializationOptions& options);
    static FWidgetSerializationResult DeserializeWidgetNode(const json& widgetJson);
};

} // namespace ImWidgetV4Editor
