#include "TitleBarView.h"
#include <imwidgetv4/core/Slot.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>
#include <nlohmann/json.hpp>

namespace {
    using json = nlohmann::ordered_json;

    json ParseGeneratedJson(const char* text)
    {
        return json::parse(text);
    }
} // namespace

namespace ImWidgetSDKBuilder {

TitleBarView::TitleBarView()
{
    SetName("TitleBarView");
}

std::shared_ptr<ImWidgetV4::ImWidget> TitleBarView::RebuildWidget()
{
    //===Auto Gen Begin=== (RebuildWidget)
    RootTitleBar = std::make_shared<ImWidgetV4::ImTitleBar>();
    RootTitleBar->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTitleBar","Properties":{"ImTitleBar::ShowSystemButtons":true,"ImTitleBar::ShowMinimizeButton":true,"ImTitleBar::ShowMaximizeButton":true,"ImTitleBar::ShowCloseButton":true,"ImTitleBar::DragRegionMinWidth":-1.0,"ImTitleBar::Style":{"Type":"FTitleBarStyle","Properties":{"FTitleBarStyle::BackgroundColor":[28,33,41,255],"FTitleBarStyle::BorderColor":[16,19,24,255],"FTitleBarStyle::BorderThickness":1.0,"FTitleBarStyle::Padding":{"Type":"FMargin","Properties":{"FMargin::Left":0.0,"FMargin::Right":0.0,"FMargin::Top":0.0,"FMargin::Bottom":0.0}},"FTitleBarStyle::ItemSpacing":6.0,"FTitleBarStyle::Height":34.0,"FTitleBarStyle::DragRegionMinWidth":34.0,"FTitleBarStyle::SystemButtonSize":34.0,"FTitleBarStyle::SystemButtonSpacing":0.0,"FTitleBarStyle::SystemButtonGlyphColor":[244,247,251,255],"FTitleBarStyle::HoveredSystemButtonColor":[255,255,255,24],"FTitleBarStyle::PressedSystemButtonColor":[255,255,255,40],"FTitleBarStyle::CloseButtonHoveredColor":[212,58,76,224],"FTitleBarStyle::CloseButtonPressedColor":[188,46,66,240],"FTitleBarStyle::MinDesiredSize":[240.0,34.0]}},"ImWidget::Name":"RootTitleBar","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    TitleBarIconSlot = std::make_shared<ImWidgetV4::ImHorizontalBox>();
    TitleBarIconSlot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImHorizontalBox","Properties":{"ImHorizontalBox::Spacing":0.0,"ImWidget::Name":"TitleBarIconSlot","ImWidget::Visible":true,"ImWidget::HitTestVisible":false,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    TitleBarIcon = std::make_shared<ImWidgetV4::ImImage>();
    TitleBarIcon->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImImage","Properties":{"ImImage::DesiredSize":[18.0,18.0],"ImImage::BackgroundColor":[0,0,0,0],"ImImage::BorderColor":[0,0,0,0],"ImImage::BorderThickness":0.0,"ImImage::CornerRadius":0.0,"ImImage::Tint":[238,242,247,255],"ImImage::StretchMode":"KeepAspect","ImWidget::Name":"TitleBarIcon","ImWidget::Visible":true,"ImWidget::HitTestVisible":false,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));
    ProjectTitleText = std::make_shared<ImWidgetV4::ImTextBlock>();
    ProjectTitleText->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImTextBlock","Properties":{"ImTextBlock::Text":"ImWidget SDK Builder","ImTextBlock::TextColor":[238,242,247,255],"ImTextBlock::FontSize":16.0,"ImTextBlock::WrapText":false,"ImTextBlock::TextAlignment":"Left","ImTextBlock::VerticalAlignment":"Center","ImWidget::Name":"ProjectTitleText","ImWidget::Visible":true,"ImWidget::HitTestVisible":true,"ImWidget::SupportsKeyboardFocus":false}})IMWJSON"));

    RootTitleBar->AddLeadingItem(TitleBarIconSlot);
    TitleBarIconSlot->AddChild(TitleBarIcon);
    if (auto* slot = TitleBarIconSlot->GetSlotForChild(TitleBarIcon)) {
        slot->FromJson(ParseGeneratedJson(R"IMWJSON({"Type":"ImBoxSlot","Properties":{"ImBoxSlot::FillCoefficient":0.0,"ImPaddingSlot::PaddingLeft":4.0,"ImPaddingSlot::PaddingRight":4.0,"ImPaddingSlot::PaddingTop":0.0,"ImPaddingSlot::PaddingBottom":0.0}})IMWJSON"));
    }
    RootTitleBar->AddLeadingItem(ProjectTitleText);

    return RootTitleBar;
    //===Auto Gen End=== (RebuildWidget)
}

} // namespace ImWidgetSDKBuilder
