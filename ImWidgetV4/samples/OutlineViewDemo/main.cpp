#include <imwidgetv4/app/ApplicationHost.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CheckBox.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/OutlineView.h>
#include <imwidgetv4\widgets/TextBlock.h>
#include <imwidgetv4\widgets/TextOutlineView.h>
#include <imwidgetv4\widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>
#include <unordered_map>

using namespace ImWidgetV4;

namespace {

std::shared_ptr<ImWidget> MakeCustomRow(
    const std::string& label,
    const std::shared_ptr<ImTextBlock>& statusText)
{
    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(8.0f);

    auto text = std::make_shared<ImTextBlock>();
    text->SetText(label);
    text->SetTextColor(FColor::White);
    row->AddChildFill(text, 1.0f);

    auto enabled = std::make_shared<ImCheckBox>();
    enabled->SetLabel("Enabled");
    enabled->SetChecked(true);
    enabled->OnCheckStateChanged.AddLambda([statusText, label](ImCheckBox&, bool checked) {
        statusText->SetText("Status: " + label + (checked ? " enabled" : " disabled"));
    });
    row->AddChild(enabled);

    auto action = std::make_shared<ImButton>();
    action->SetText("Run");
    action->OnClicked.AddLambda([statusText, label](ImButton&) {
        statusText->SetText("Status: invoked " + label);
    });
    row->AddChild(action);

    return row;
}

class FOutlineViewDemoHostDelegate : public IApplicationHostDelegate {
public:
    FApplicationHostConfig GetHostConfig() const override
    {
        FApplicationHostConfig config;
        config.Title = "Outline View Demo - ImWidgetV4";
        config.InitialWidth = 1180;
        config.InitialHeight = 760;
#if defined(_WIN32)
        config.IniSettingsPath = Samples::GetDefaultSampleImGuiIniPath(L"OutlineViewDemo.ini");
#endif
        return config;
    }

    void ConfigureApplication(ImApplication& application) override
    {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(12.0f);

        auto title = std::make_shared<ImTextBlock>();
        title->SetText("Outline View Demo");
        title->SetFontSize(28.0f);
        title->SetTextColor(FColor::White);
        root->AddChild(title, FMargin(18.0f, 18.0f, 18.0f, 0.0f));

        auto subtitle = std::make_shared<ImTextBlock>();
        subtitle->SetText("Left: pure text outline with self-managed drawing and navigation. Right: custom-content outline where each row can host retained widgets.");
        subtitle->SetWrapText(true);
        subtitle->SetTextColor(FColor::FromBytes(214, 222, 234));
        root->AddChild(subtitle, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

        auto status = std::make_shared<ImTextBlock>();
        status->SetText("Status: click an item, use arrow keys to navigate, or right-click a row to request a context action.");
        status->SetWrapText(true);
        status->SetTextColor(FColor::FromBytes(160, 214, 190));
        root->AddChild(status, FMargin(18.0f, 0.0f, 18.0f, 0.0f));

        auto contentRow = std::make_shared<ImHorizontalBox>();
        contentRow->SetSpacing(14.0f);

        auto textColumn = std::make_shared<ImVerticalBox>();
        textColumn->SetSpacing(8.0f);

        auto textColumnTitle = std::make_shared<ImTextBlock>();
        textColumnTitle->SetText("ImTextOutlineView");
        textColumnTitle->SetTextColor(FColor::FromBytes(255, 214, 102));
        textColumn->AddChild(textColumnTitle);

        auto textOutline = std::make_shared<ImTextOutlineView>();
        ImTextOutlineItem* assets = textOutline->AddRootItem("Assets");
        ImTextOutlineItem* textures = textOutline->AddChildItem(assets, "Textures");
        textOutline->AddChildItem(textures, "Icons");
        textOutline->AddChildItem(textures, "Materials");
        ImTextOutlineItem* scripts = textOutline->AddChildItem(assets, "Scripts");
        textOutline->AddChildItem(scripts, "AI");
        textOutline->AddChildItem(scripts, "UI");
        ImTextOutlineItem* docs = textOutline->AddRootItem("Docs");
        textOutline->AddChildItem(docs, "Roadmap.md");
        textOutline->AddChildItem(docs, "SnapshotGuide.md");
        textOutline->ExpandAll();
        textColumn->AddChildFill(textOutline, 1.0f);

        auto customColumn = std::make_shared<ImVerticalBox>();
        customColumn->SetSpacing(8.0f);

        auto customColumnTitle = std::make_shared<ImTextBlock>();
        customColumnTitle->SetText("ImOutlineView");
        customColumnTitle->SetTextColor(FColor::FromBytes(255, 214, 102));
        customColumn->AddChild(customColumnTitle);

        auto customOutline = std::make_shared<ImOutlineView>();
        std::unordered_map<ImOutlineItem*, std::string> customLabels;

        ImOutlineItem* rendering = customOutline->AddRootItem(MakeCustomRow("Rendering", status));
        customLabels[rendering] = "Rendering";
        ImOutlineItem* shadows = customOutline->AddChildItem(rendering, MakeCustomRow("Shadows", status));
        customLabels[shadows] = "Shadows";
        ImOutlineItem* postProcess = customOutline->AddChildItem(rendering, MakeCustomRow("Post Process", status));
        customLabels[postProcess] = "Post Process";

        ImOutlineItem* tools = customOutline->AddRootItem(MakeCustomRow("Tools", status));
        customLabels[tools] = "Tools";
        ImOutlineItem* profiler = customOutline->AddChildItem(tools, MakeCustomRow("Profiler", status));
        customLabels[profiler] = "Profiler";
        ImOutlineItem* inspector = customOutline->AddChildItem(tools, MakeCustomRow("Inspector", status));
        customLabels[inspector] = "Inspector";
        customOutline->ExpandAll();
        customColumn->AddChildFill(customOutline, 1.0f);

        textOutline->OnSelectionChanged.AddLambda([status](ImTextOutlineView&, ImTextOutlineItem* item) {
            status->SetText(item != nullptr
                ? "Status: text outline selected \"" + item->Text + "\""
                : "Status: text outline selection cleared");
        });
        textOutline->OnItemContextMenuRequested.AddLambda([status](ImTextOutlineView&, ImTextOutlineItem& item, FVector2) {
            status->SetText("Status: text outline requested context menu for \"" + item.Text + "\"");
        });

        customOutline->OnSelectionChanged.AddLambda([status, customLabels](ImOutlineView&, ImOutlineItem* item) mutable {
            if (item == nullptr) {
                status->SetText("Status: custom outline selection cleared");
                return;
            }

            const auto it = customLabels.find(item);
            status->SetText("Status: custom outline selected \"" + (it != customLabels.end() ? it->second : std::string("Unknown")) + "\"");
        });
        customOutline->OnItemContextMenuRequested.AddLambda([status, customLabels](ImOutlineView&, ImOutlineItem& item, FVector2) mutable {
            const auto it = customLabels.find(&item);
            status->SetText("Status: custom outline requested context menu for \"" + (it != customLabels.end() ? it->second : std::string("Unknown")) + "\"");
        });

        contentRow->AddChildFill(textColumn, 1.0f);
        contentRow->AddChildFill(customColumn, 1.0f);
        root->AddChildFill(contentRow, 1.0f, FMargin(18.0f, 0.0f, 18.0f, 18.0f));

        application.SetRootWidget(root);
    }
};

} // namespace

namespace ImWidgetV4 {

std::shared_ptr<IApplicationHostDelegate> CreateApplicationHostDelegate()
{
    return std::make_shared<FOutlineViewDemoHostDelegate>();
}

} // namespace ImWidgetV4
