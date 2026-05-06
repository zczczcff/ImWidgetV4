#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/CanvasPanel.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include <Windows.h>

using namespace ImWidgetV4;

namespace {

FSnapshotOptions MakeSnapshotOptions(const FFrameInfo& frameInfo)
{
    FSnapshotOptions options;
    options.Width = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.X)));
    options.Height = (std::max)(1, static_cast<int>(std::lround(frameInfo.ViewportSize.Y)));
    options.ClearColor = FColor::FromBytes(15, 18, 24);
    return options;
}

FScrollBoxStyle MakeScrollStyle()
{
    FScrollBoxStyle style;
    style.BackgroundColor = FColor::FromBytes(21, 26, 34);
    style.BorderColor = FColor::FromBytes(49, 63, 79);
    style.ScrollbarTrackColor = FColor::FromBytes(39, 50, 63);
    style.ScrollbarThumbColor = FColor::FromBytes(102, 126, 156);
    style.ScrollbarThumbHoveredColor = FColor::FromBytes(137, 170, 209);
    style.Padding = FMargin(10.0f);
    style.ScrollbarThickness = 12.0f;
    style.ScrollbarPadding = 4.0f;
    style.WheelScrollStep = 42.0f;
    return style;
}

std::shared_ptr<ImTextBlock> MakeSectionTitle(const std::string& text)
{
    auto title = std::make_shared<ImTextBlock>();
    title->SetText(text);
    title->SetFontSize(18.0f);
    title->SetTextColor(FColor::FromBytes(255, 214, 102));
    return title;
}

std::shared_ptr<ImTextBlock> MakeSectionDescription(const std::string& text)
{
    auto description = std::make_shared<ImTextBlock>();
    description->SetText(text);
    description->SetWrapText(true);
    description->SetTextColor(FColor::FromBytes(214, 222, 234));
    return description;
}

std::shared_ptr<ImButton> MakePrimaryButton(const std::string& text)
{
    auto button = std::make_shared<ImButton>();
    button->SetText(text);
    button->SetStyle(FButtonStyle::CreatePrimary());
    return button;
}

std::shared_ptr<ImButton> MakeDangerButton(const std::string& text)
{
    auto button = std::make_shared<ImButton>();
    button->SetText(text);
    button->SetStyle(FButtonStyle::CreateDanger());
    return button;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"ScrollBox Demo - ImWidgetV4",
        1260,
        860);

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Samples::GetDefaultSampleImGuiIniPath(L"ScrollBoxDemo.ini"));
    backend->SetApplication(app.get());

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(10.0f);

    auto title = std::make_shared<ImTextBlock>();
    title->SetText("ScrollBox Demo - vertical, horizontal, and dual-axis scrolling");
    title->SetTextColor(FColor::White);
    title->SetFontSize(24.0f);
    root->AddChild(title, FMargin(14.0f, 14.0f, 14.0f, 0.0f));

    auto description = std::make_shared<ImTextBlock>();
    description->SetText("This standalone demo shows three common ScrollBox scenarios: a long vertical list, a wide horizontal strip, and an oversized canvas that needs both axes. The toolbar buttons use ScrollToWidget so you can test programmatic navigation without touching internal state.");
    description->SetWrapText(true);
    description->SetTextColor(FColor::FromBytes(214, 222, 234));
    root->AddChild(description, FMargin(14.0f, 0.0f, 14.0f, 0.0f));

    auto toolbar = std::make_shared<ImHorizontalBox>();
    toolbar->SetSpacing(8.0f);

    auto jumpListButton = std::make_shared<ImButton>();
    jumpListButton->SetText("Jump List Target");
    toolbar->AddChild(jumpListButton);

    auto jumpStripButton = std::make_shared<ImButton>();
    jumpStripButton->SetText("Jump Strip Target");
    toolbar->AddChild(jumpStripButton);

    auto jumpCanvasCenterButton = std::make_shared<ImButton>();
    jumpCanvasCenterButton->SetText("Jump Canvas Center");
    toolbar->AddChild(jumpCanvasCenterButton);

    auto jumpCanvasTargetButton = MakePrimaryButton("Jump Canvas Target");
    toolbar->AddChild(jumpCanvasTargetButton);

    auto resetButton = std::make_shared<ImButton>();
    resetButton->SetText("Reset Scroll");
    toolbar->AddChild(resetButton);

    auto exportSnapshotButton = std::make_shared<ImButton>();
    exportSnapshotButton->SetText("Export Snapshot");
    toolbar->AddChild(exportSnapshotButton);

    root->AddChild(toolbar, FMargin(14.0f, 0.0f, 14.0f, 0.0f));

    auto statusText = std::make_shared<ImTextBlock>();
    statusText->SetText("Scroll with the mouse wheel, then use the buttons above to jump between key widgets.");
    statusText->SetTextColor(FColor::FromBytes(220, 228, 236));
    statusText->SetWrapText(true);
    root->AddChild(statusText, FMargin(14.0f, 0.0f, 14.0f, 0.0f));

    auto body = std::make_shared<ImHorizontalBox>();
    body->SetSpacing(14.0f);
    root->AddChildFill(body, 1.0f, FMargin(14.0f, 0.0f, 14.0f, 14.0f));

    auto leftColumn = std::make_shared<ImVerticalBox>();
    leftColumn->SetSpacing(10.0f);
    body->AddChildFill(leftColumn, 0.95f);

    auto rightColumn = std::make_shared<ImVerticalBox>();
    rightColumn->SetSpacing(10.0f);
    body->AddChildFill(rightColumn, 1.35f);

    const FScrollBoxStyle scrollStyle = MakeScrollStyle();

    auto verticalTitle = MakeSectionTitle("Vertical List");
    leftColumn->AddChild(verticalTitle);

    auto verticalDescription = MakeSectionDescription("A single-content VerticalBox inside ScrollBox. The list content is taller than the viewport, so only the vertical scrollbar appears.");
    leftColumn->AddChild(verticalDescription);

    auto verticalScrollBox = std::make_shared<ImScrollBox>();
    verticalScrollBox->SetStyle(scrollStyle);

    auto verticalContent = std::make_shared<ImVerticalBox>();
    verticalContent->SetSpacing(6.0f);

    std::shared_ptr<ImButton> listTargetButton;
    for (int index = 0; index < 18; ++index) {
        auto rowButton = std::make_shared<ImButton>();
        rowButton->SetText("Task Row " + std::to_string(index + 1));
        if (index == 14) {
            rowButton->SetStyle(FButtonStyle::CreatePrimary());
            listTargetButton = rowButton;
        }
        rowButton->OnClicked.AddLambda([&, index](ImButton&) {
            statusText->SetText("Clicked vertical list row " + std::to_string(index + 1) + ".");
        });
        verticalContent->AddChild(rowButton, FMargin(6.0f, 6.0f, index == 0 ? 6.0f : 0.0f, 0.0f));
    }

    verticalScrollBox->SetContent(verticalContent);
    leftColumn->AddChildFill(verticalScrollBox, 1.0f);

    auto horizontalTitle = MakeSectionTitle("Horizontal Strip");
    leftColumn->AddChild(horizontalTitle, FMargin(0.0f, 0.0f, 6.0f, 0.0f));

    auto horizontalDescription = MakeSectionDescription("A wide HorizontalBox with long card labels. This section keeps height compact and lets you verify horizontal-only scrolling.");
    leftColumn->AddChild(horizontalDescription);

    auto horizontalScrollBox = std::make_shared<ImScrollBox>();
    horizontalScrollBox->SetStyle(scrollStyle);

    auto horizontalContent = std::make_shared<ImHorizontalBox>();
    horizontalContent->SetSpacing(12.0f);

    std::shared_ptr<ImButton> stripTargetButton;
    const std::vector<std::string> stripLabels = {
        "Prototype Layout",
        "Review Input Routing",
        "Tune Splitter Ratios",
        "Capture Snapshot",
        "Verify Modal Flow",
        "Document Theme Pack",
        "Ship Scroll Target"
    };

    for (std::size_t index = 0; index < stripLabels.size(); ++index) {
        auto cardButton = std::make_shared<ImButton>();
        cardButton->SetText(stripLabels[index]);
        if (index == stripLabels.size() - 1) {
            cardButton->SetStyle(FButtonStyle::CreatePrimary());
            stripTargetButton = cardButton;
        }
        cardButton->OnClicked.AddLambda([&, index](ImButton&) {
            statusText->SetText("Clicked horizontal strip card " + std::to_string(index + 1) + ".");
        });
        horizontalContent->AddChild(cardButton, FMargin(index == 0 ? 8.0f : 0.0f, 0.0f, 8.0f, 8.0f));
    }

    horizontalScrollBox->SetContent(horizontalContent);
    leftColumn->AddChildFill(horizontalScrollBox, 0.78f);

    auto canvasTitle = MakeSectionTitle("Dual-Axis Canvas");
    rightColumn->AddChild(canvasTitle);

    auto canvasDescription = MakeSectionDescription("The content below is a CanvasPanel larger than the viewport in both axes. It demonstrates clipping, both scrollbars, and ScrollToWidget against arbitrary retained-mode geometry.");
    rightColumn->AddChild(canvasDescription);

    auto canvasScrollBox = std::make_shared<ImScrollBox>();
    canvasScrollBox->SetStyle(scrollStyle);

    auto canvas = std::make_shared<ImCanvasPanel>();
    canvas->SetDesiredSize(FVector2(1680.0f, 1220.0f));

    auto stageTitle = std::make_shared<ImTextBlock>();
    stageTitle->SetText("Oversized Canvas Content");
    stageTitle->SetFontSize(28.0f);
    stageTitle->SetTextColor(FColor::FromBytes(255, 214, 102));
    canvas->AddChildAt(stageTitle, FVector2(0.03f, 0.04f));

    auto stageHint = std::make_shared<ImTextBlock>();
    stageHint->SetText("Widgets are intentionally spread across the surface so the view needs horizontal and vertical scrolling to inspect everything.");
    stageHint->SetWrapText(true);
    stageHint->SetTextColor(FColor::FromBytes(214, 222, 234));
    canvas->AddChildAt(stageHint, FVector2(0.03f, 0.10f), FVector2(0.44f, 0.08f));

    auto upperLeft = MakePrimaryButton("Top Left Node");
    canvas->AddChildAt(upperLeft, FVector2(0.06f, 0.24f));

    auto centerCard = MakePrimaryButton("Center Node");
    canvas->AddChildAt(centerCard, FVector2(0.46f, 0.44f));

    auto farRight = MakePrimaryButton("Far Right Node");
    canvas->AddChildAt(farRight, FVector2(0.83f, 0.17f));

    auto lowerLeft = MakePrimaryButton("Lower Section");
    canvas->AddChildAt(lowerLeft, FVector2(0.14f, 0.81f));

    auto canvasTargetButton = MakeDangerButton("Bottom Right Target");
    canvas->AddChildAt(canvasTargetButton, FVector2(0.85f, 0.89f));

    auto canvasNote = std::make_shared<ImTextBlock>();
    canvasNote->SetText("ScrollToWidget runs against the current arranged geometry, so this demo behaves naturally after the first rendered frame.");
    canvasNote->SetWrapText(true);
    canvasNote->SetTextColor(FColor::FromBytes(190, 220, 255));
    canvas->AddChildAt(canvasNote, FVector2(0.58f, 0.60f), FVector2(0.27f, 0.11f));

    canvasScrollBox->SetContent(canvas);
    rightColumn->AddChildFill(canvasScrollBox, 1.0f);

    app->SetRootWidget(root);

    const std::filesystem::path snapshotPath =
        std::filesystem::absolute(std::filesystem::path("artifacts") / "snapshots" / "scroll_box_demo.png");
    bool bPendingSnapshotExport = false;

    jumpListButton->OnClicked.AddLambda([&](ImButton&) {
        verticalScrollBox->ScrollToWidget(listTargetButton);
        statusText->SetText("Jumped to the highlighted row in the vertical list.");
    });

    jumpStripButton->OnClicked.AddLambda([&](ImButton&) {
        horizontalScrollBox->ScrollToWidget(stripTargetButton, true);
        statusText->SetText("Jumped to the final card in the horizontal strip.");
    });

    jumpCanvasCenterButton->OnClicked.AddLambda([&](ImButton&) {
        canvasScrollBox->ScrollToWidget(centerCard, true);
        statusText->SetText("Centered the middle canvas node.");
    });

    jumpCanvasTargetButton->OnClicked.AddLambda([&](ImButton&) {
        canvasScrollBox->ScrollToWidget(canvasTargetButton);
        statusText->SetText("Jumped to the bottom-right canvas target.");
    });

    resetButton->OnClicked.AddLambda([&](ImButton&) {
        verticalScrollBox->ScrollToStart();
        horizontalScrollBox->ScrollToStart();
        canvasScrollBox->ScrollToStart();
        statusText->SetText("Reset all ScrollBox views back to their origin.");
    });

    exportSnapshotButton->OnClicked.AddLambda([&](ImButton&) {
        bPendingSnapshotExport = true;
        statusText->SetText("Snapshot export scheduled for the end of this frame.");
    });

    listTargetButton->OnClicked.AddLambda([&](ImButton&) {
        statusText->SetText("Highlighted vertical list target clicked.");
    });

    stripTargetButton->OnClicked.AddLambda([&](ImButton&) {
        statusText->SetText("Highlighted horizontal strip target clicked.");
    });

    canvasTargetButton->OnClicked.AddLambda([&](ImButton&) {
        statusText->SetText("Bottom-right canvas target clicked.");
    });

    backend->SetPostFrameCallback([&](const FFrameInfo& frameInfo) {
        if (!bPendingSnapshotExport) {
            return;
        }

        bPendingSnapshotExport = false;

        std::error_code directoryError;
        std::filesystem::create_directories(snapshotPath.parent_path(), directoryError);

        FFrameContext snapshotFrameContext;
        snapshotFrameContext.FrameInfo = frameInfo;

        const bool bExported =
            !directoryError &&
            app->ExportSnapshotToPng(snapshotPath, snapshotFrameContext, MakeSnapshotOptions(frameInfo));

        if (bExported) {
            statusText->SetText("Snapshot exported to: " + snapshotPath.string());
        } else {
            std::string message = "Snapshot export failed: " + snapshotPath.string();
            if (directoryError) {
                message += " (" + directoryError.message() + ")";
            }
            statusText->SetText(message);
        }
    });

    backend->Run();
    backend->Shutdown();
    return 0;
}


