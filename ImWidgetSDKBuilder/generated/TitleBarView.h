#pragma once

#include <imwidgetv4/widgets/UserWidget.h>
#include <memory>

namespace ImWidgetV4 {
    class ImWidget;
    class ImHorizontalBox;
    class ImImage;
    class ImTextBlock;
    class ImTitleBar;
} // namespace ImWidgetV4

namespace ImWidgetSDKBuilder {

class TitleBarView : public ImWidgetV4::ImUserWidget {
public:
    TitleBarView();

    //===Auto Gen Begin=== (Public Members)
    std::shared_ptr<ImWidgetV4::ImTitleBar> RootTitleBar;
    std::shared_ptr<ImWidgetV4::ImHorizontalBox> TitleBarIconSlot;
    std::shared_ptr<ImWidgetV4::ImImage> TitleBarIcon;
    std::shared_ptr<ImWidgetV4::ImTextBlock> ProjectTitleText;
    //===Auto Gen End=== (Public Members)

protected:
    std::shared_ptr<ImWidgetV4::ImWidget> RebuildWidget() override;

private:
    //===Auto Gen Begin=== (Private Members)
    //===Auto Gen End=== (Private Members)
};

} // namespace ImWidgetSDKBuilder
