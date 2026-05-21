#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
    class ImWidget;
    class ImTitleBar;
} // namespace ImWidgetV4

namespace ImWidgetSDKBuilder {

class TitleBarView : public ImWidgetV4::ImUserWidget {
public:
    TitleBarView();

protected:
    std::shared_ptr<ImWidgetV4::ImWidget> RebuildWidget() override;

private:
    //===Auto Gen Begin=== (Members)
    std::shared_ptr<ImWidgetV4::ImTitleBar> RootTitleBar;
    //===Auto Gen End=== (Members)
};

} // namespace ImWidgetSDKBuilder
