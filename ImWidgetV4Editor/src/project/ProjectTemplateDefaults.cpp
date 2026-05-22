#include "ProjectTemplateDefaults.h"

#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/TitleBar.h>

namespace ImWidgetV4Editor {

using namespace ImWidgetV4;

std::shared_ptr<ImWidget> BuildDefaultStartupRoot()
{
    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetName("RootCanvas");
    canvas->SetDesiredSize(FVector2(1280.0f, 720.0f));

    auto title = std::make_shared<ImTextBlock>();
    title->SetName("TitleText");
    title->SetText("ImWidgetV4 App");
    title->SetFontSize(32.0f);
    title->SetWrapText(false);
    if (auto* slot = canvas->AddChildAt(title, FVector2(0.08f, 0.08f))) {
        slot->SetAutoSize(true);
    }

    auto button = std::make_shared<ImButton>();
    button->SetName("PrimaryButton");
    button->SetText("Action");
    if (auto* slot = canvas->AddChildAt(button, FVector2(0.08f, 0.20f))) {
        slot->SetAutoSize(true);
    }

    return canvas;
}

std::shared_ptr<ImWidget> BuildDefaultTitleBarRoot(const std::string& projectName)
{
    auto titleBar = std::make_shared<ImTitleBar>();
    titleBar->SetName("RootTitleBar");
    titleBar->SetShowSystemButtons(true);

    auto titleText = std::make_shared<ImTextBlock>();
    titleText->SetName("ProjectTitleText");
    titleText->SetText(projectName.empty() ? std::string("Application") : projectName);
    titleText->SetWrapText(false);
    titleBar->AddLeadingItem(titleText);

    return titleBar;
}

} // namespace ImWidgetV4Editor
