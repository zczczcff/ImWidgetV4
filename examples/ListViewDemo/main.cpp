#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ListView.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <Windows.h>
#include <memory>
#include <string>
#include <vector>

using namespace ImWidgetV4;

namespace {

struct FDemoRowItem {
    std::string Title;
    bool bEnabled = false;
};

std::shared_ptr<ImWidget> MakeLargeListRow(std::size_t index)
{
    auto text = std::make_shared<ImTextBlock>();
    text->SetText("Item " + std::to_string(index) + " - generated on demand");
    text->SetTextColor(FColor::White);
    return text;
}

std::shared_ptr<ImWidget> MakeCustomRow(
    std::size_t index,
    std::vector<FDemoRowItem>& items,
    const std::shared_ptr<ImTextBlock>& statusText,
    const std::shared_ptr<ImListView>& owner)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);

    auto label = std::make_shared<ImTextBlock>();
    label->SetText(items[index].Title);
    label->SetTextColor(FColor::White);
    row->AddChildFill(label, 1.0f);

    auto checkBox = std::make_shared<ImCheckBox>();
    checkBox->SetLabel("Enabled");
    checkBox->SetChecked(items[index].bEnabled);
    checkBox->OnCheckStateChanged.AddLambda([index, &items, statusText](ImCheckBox&, bool checked) {
        items[index].bEnabled = checked;
        statusText->SetText("Status: toggled " + items[index].Title + (checked ? " on" : " off"));
    });
    row->AddChild(checkBox);

    auto action = std::make_shared<ImButton>();
    action->SetText("Ping");
    action->OnClicked.AddLambda([index, &items, statusText, owner](ImButton&) {
        statusText->SetText("Status: invoked action on " + items[index].Title);
        owner->SetSelectedIndex(static_cast<int>(index));
    });
    row->AddChild(action);

    return row;
}

} // namespace

int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"ListView Demo - ImWidgetV4",
        1180,
        760
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"ListViewDemo.ini"));
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(12.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("ListView Demo");
    title->SetFontSize(28.0f);
    title->SetTextColor(FColor::White);
    root->AddChild(title, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

    auto subtitle = std::make_shared<ImTextBlock>();
    subtitle->SetText("Left: a 1200-row generated list that only realizes visible rows. Right: custom rows showing selection-before-routing semantics.");
    subtitle->SetWrapText(true);
    subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(subtitle, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto status = std::make_shared<ImTextBlock>();
    status->SetText("Status: select a row, right-click for a context notification, or use the embedded controls on the custom list.");
    status->SetWrapText(true);
    status->SetTextColor(FColor::FromBytes(160, 214, 190));
    root->AddChild(status, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

    auto columns = std::make_shared<ImHorizontalBox>();
    columns->SetSpacing(14.0f);

    auto generatedColumn = std::make_shared<ImVerticalBox>();
    generatedColumn->SetSpacing(8.0f);

    auto generatedTitle = std::make_shared<ImTextBlock>();
    generatedTitle->SetText("Large Generated List");
    generatedTitle->SetTextColor(FColor::FromBytes(255, 214, 102));
    generatedColumn->AddChild(generatedTitle);

    auto generatedList = std::make_shared<ImListView>();
    generatedList->SetItemCount(1200);
    generatedList->SetOnGenerateRow([](std::size_t index) {
        return MakeLargeListRow(index);
    });
    generatedList->OnSelectionChanged.AddLambda([status](ImListView&, int index) {
        status->SetText(index >= 0
            ? "Status: generated list selected index " + std::to_string(index)
            : "Status: generated list selection cleared");
    });
    generatedList->OnItemContextMenuRequested.AddLambda([status](ImListView&, int index, FVector2) {
        status->SetText("Status: generated list requested context menu for index " + std::to_string(index));
    });
    generatedColumn->AddChildFill(generatedList, 1.0f);

    auto customColumn = std::make_shared<ImVerticalBox>();
    customColumn->SetSpacing(8.0f);

    auto customTitle = std::make_shared<ImTextBlock>();
    customTitle->SetText("Custom Row List");
    customTitle->SetTextColor(FColor::FromBytes(255, 214, 102));
    customColumn->AddChild(customTitle);

    auto customList = std::make_shared<ImListView>();
    auto items = std::make_shared<std::vector<FDemoRowItem>>(std::vector<FDemoRowItem> {
        {"Project Overview", true},
        {"Renderer Settings", false},
        {"Input Mapping", true},
        {"Localization", false},
        {"Asset Import", true},
        {"Automation Hooks", false},
        {"Snapshot Pipeline", true},
        {"Window Chrome", false}
    });
    customList->SetItemCount(items->size());
    customList->SetOnGenerateRow([items, status, customList](std::size_t index) {
        return MakeCustomRow(index, *items, status, customList);
    });
    customList->OnSelectionChanged.AddLambda([status, items](ImListView&, int index) {
        status->SetText(index >= 0
            ? "Status: custom list selected \"" + (*items)[static_cast<std::size_t>(index)].Title + "\""
            : "Status: custom list selection cleared");
    });
    customList->OnItemContextMenuRequested.AddLambda([status, items](ImListView&, int index, FVector2) {
        if (index >= 0) {
            status->SetText("Status: custom list requested context menu for \"" + (*items)[static_cast<std::size_t>(index)].Title + "\"");
        }
    });
    customColumn->AddChildFill(customList, 1.0f);

    columns->AddChildFill(generatedColumn, 1.0f);
    columns->AddChildFill(customColumn, 1.0f);
    root->AddChildFill(columns, 1.0f, FMargin(18.0f, 0.0f, 18.0f, 18.0f));

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();
    return 0;
}
