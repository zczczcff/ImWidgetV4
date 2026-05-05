#include <Windows.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/platform/Win32DX11Backend.h>
#include <imwidgetv4/widgets/HorizontalBox.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include "../DemoPaths.h"
#include <memory>
#include <string>
#include <array>
#include <vector>

using namespace ImWidgetV4;

namespace {

std::vector<std::uint8_t> BuildDemoTexture(int width, int height)
{
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);
    const int widthDenominator = width > 1 ? (width - 1) : 1;
    const int heightDenominator = height > 1 ? (height - 1) : 1;
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const std::size_t offset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4U;
            pixels[offset] = static_cast<std::uint8_t>(40 + (x * 180) / widthDenominator);
            pixels[offset + 1] = static_cast<std::uint8_t>(90 + (y * 120) / heightDenominator);
            pixels[offset + 2] = static_cast<std::uint8_t>(220 - (x * 100) / widthDenominator);
            pixels[offset + 3] = 255;
        }
    }

    const int stripeStart = width / 3;
    const int stripeEnd = stripeStart + (width / 8 > 2 ? width / 8 : 2);
    for (int y = 1; y < height - 1; ++y) {
        for (int x = stripeStart; x < stripeEnd; ++x) {
            const std::size_t offset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4U;
            pixels[offset] = 255;
            pixels[offset + 1] = 242;
            pixels[offset + 2] = 164;
        }
    }

    return pixels;
}

std::shared_ptr<ImTextBlock> MakeLabel(const std::string& text, const FColor& color = FColor::White)
{
    auto label = std::make_shared<ImTextBlock>();
    label->SetText(text);
    label->SetWrapText(true);
    label->SetTextColor(color);
    return label;
}

std::shared_ptr<ImVerticalBox> MakeImageCard(
    const std::string& title,
    const std::string& caption,
    const std::shared_ptr<ImImage>& image)
{
    auto card = std::make_shared<ImVerticalBox>();
    card->SetSpacing(8.0f);
    card->AddChild(MakeLabel(title, FColor::FromBytes(255, 214, 102)));
    card->AddChild(image);
    card->AddChild(MakeLabel(caption, FColor::FromBytes(214, 222, 234)));
    return card;
}

constexpr std::array<std::pair<ECoreIcon, const char*>, 20> GCoreIcons = {{
    {ECoreIcon::Save, "Save"},
    {ECoreIcon::Folder, "Folder"},
    {ECoreIcon::File, "File"},
    {ECoreIcon::Copy, "Copy"},
    {ECoreIcon::Paste, "Paste"},
    {ECoreIcon::Cut, "Cut"},
    {ECoreIcon::Trash, "Trash"},
    {ECoreIcon::Undo, "Undo"},
    {ECoreIcon::Redo, "Redo"},
    {ECoreIcon::Search, "Search"},
    {ECoreIcon::Settings, "Settings"},
    {ECoreIcon::Add, "Add"},
    {ECoreIcon::Remove, "Remove"},
    {ECoreIcon::ArrowUp, "ArrowUp"},
    {ECoreIcon::ArrowDown, "ArrowDown"},
    {ECoreIcon::Download, "Download"},
    {ECoreIcon::Upload, "Upload"},
    {ECoreIcon::Lock, "Lock"},
    {ECoreIcon::Unlock, "Unlock"},
    {ECoreIcon::View, "View"}
}};

std::shared_ptr<ImVerticalBox> MakeCoreIconCell(
    const FImageBrush& brush,
    const std::string& label,
    const FColor& labelColor)
{
    auto cell = std::make_shared<ImVerticalBox>();
    cell->SetSpacing(6.0f);

    auto image = std::make_shared<ImImage>();
    image->SetBrush(brush);
    image->SetDesiredSize(FVector2(56.0f, 56.0f));
    image->SetBackgroundColor(FColor::FromBytes(24, 31, 40));
    image->SetCornerRadius(10.0f);

    auto text = MakeLabel(label, labelColor);
    text->SetFontSize(13.0f);

    cell->AddChild(image);
    cell->AddChild(text);
    return cell;
}

std::shared_ptr<ImVerticalBox> MakeCoreIconGrid(
    ImApplication& application,
    bool bTinted)
{
    static const std::array<FColor, 5> AccentColors = {
        FColor::FromBytes(255, 214, 102),
        FColor::FromBytes(123, 221, 255),
        FColor::FromBytes(255, 160, 122),
        FColor::FromBytes(174, 234, 119),
        FColor::FromBytes(232, 170, 255)
    };

    auto grid = std::make_shared<ImVerticalBox>();
    grid->SetSpacing(12.0f);

    for (int rowIndex = 0; rowIndex < 4; ++rowIndex) {
        auto row = std::make_shared<ImHorizontalBox>();
        row->SetSpacing(12.0f);

        for (int columnIndex = 0; columnIndex < 5; ++columnIndex) {
            const int iconIndex = rowIndex * 5 + columnIndex;
            const FColor tint = bTinted ? AccentColors[static_cast<std::size_t>(iconIndex % AccentColors.size())]
                                        : FColor::White;
            const FColor labelColor = bTinted ? tint : FColor::FromBytes(214, 222, 234);
            row->AddChild(
                MakeCoreIconCell(
                    application.GetCoreIconBrush(GCoreIcons[static_cast<std::size_t>(iconIndex)].first, tint),
                    GCoreIcons[static_cast<std::size_t>(iconIndex)].second,
                    labelColor));
        }

        grid->AddChild(row);
    }

    return grid;
}

} // namespace

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    auto backend = std::make_shared<ImWin32DX11Backend>(
        L"Image Demo - ImWidgetV4",
        1080,
        760
    );

    if (!backend->Initialize()) {
        MessageBoxW(nullptr, L"Backend initialization failed", L"Error", MB_OK | MB_ICONERROR);
        return -1;
    }

    auto app = std::make_shared<ImApplication>();
    app->SetIniSettingsPath(Examples::GetDefaultDemoImGuiIniPath(L"ImageDemo.ini"));
    backend->SetApplication(app.get());

    const ImTextureID demoTexture = app->CreateRuntimeTextureFromRgba(BuildDemoTexture(20, 12), 20, 12);

    auto root = std::make_shared<ImVerticalBox>();
    root->SetSpacing(16.0f);

    auto title = MakeLabel("Image Demo");
    title->SetFontSize(28.0f);
    root->AddChild(title, FMargin(20.0f, 20.0f, 20.0f, 0.0f));

    root->AddChild(
        MakeLabel(
            "ImImage accepts either a brush or a raw texture handle. When the brush is empty or the texture is null, it falls back to the built-in question-mark placeholder.",
            FColor::FromBytes(214, 222, 234)),
        FMargin(20.0f, 0.0f, 20.0f, 0.0f));

    auto row = std::make_shared<ImHorizontalBox>();
    row->SetSpacing(18.0f);

    auto validImage = std::make_shared<ImImage>();
    validImage->SetTexture(demoTexture, FVector2(20.0f, 12.0f));
    validImage->SetDesiredSize(FVector2(220.0f, 120.0f));

    auto fillImage = std::make_shared<ImImage>();
    fillImage->SetTexture(demoTexture, FVector2(20.0f, 12.0f));
    fillImage->SetDesiredSize(FVector2(220.0f, 120.0f));
    fillImage->SetStretchMode(EImageStretchMode::Fill);
    fillImage->SetTint(FColor::FromBytes(255, 228, 164));
    fillImage->SetBackgroundColor(FColor::FromBytes(26, 34, 42));
    fillImage->SetCornerRadius(14.0f);

    auto placeholderImage = std::make_shared<ImImage>();
    placeholderImage->SetDesiredSize(FVector2(220.0f, 120.0f));

    auto nullTextureImage = std::make_shared<ImImage>();
    nullTextureImage->SetTexture(nullptr, FVector2(20.0f, 12.0f));
    nullTextureImage->SetDesiredSize(FVector2(220.0f, 120.0f));
    nullTextureImage->SetBackgroundColor(FColor::FromBytes(28, 36, 48));

    row->AddChild(
        MakeImageCard(
            "Valid Texture / KeepAspect",
            "The 20x12 test texture keeps its aspect ratio and stays centered inside the image box.",
            validImage),
        FMargin(20.0f, 0.0f, 0.0f, 20.0f));
    row->AddChild(
        MakeImageCard(
            "Valid Texture / Fill",
            "Fill mode stretches to the full geometry so the same texture intentionally deforms.",
            fillImage),
        FMargin(0.0f, 0.0f, 0.0f, 20.0f));
    row->AddChild(
        MakeImageCard(
            "Empty Brush Fallback",
            "No brush set: ImImage asks the application for the built-in question-mark placeholder texture.",
            placeholderImage),
        FMargin(0.0f, 0.0f, 0.0f, 20.0f));
    row->AddChild(
        MakeImageCard(
            "Null Texture Fallback",
            "A brush with a null texture handle resolves to the same placeholder path as the empty image.",
            nullTextureImage),
        FMargin(0.0f, 20.0f, 0.0f, 20.0f));

    root->AddChild(row, FMargin(0.0f));

    auto iconSectionTitle = MakeLabel("Core Icons");
    iconSectionTitle->SetFontSize(24.0f);
    root->AddChild(iconSectionTitle, FMargin(20.0f, 20.0f, 20.0f, 0.0f));

    root->AddChild(
        MakeLabel(
            "The library now embeds a 20-icon atlas. The first grid uses the default white brushes, and the second grid reuses the same atlas with per-brush tint colors.",
            FColor::FromBytes(214, 222, 234)),
        FMargin(20.0f, 0.0f, 20.0f, 0.0f));

    auto whiteIconsTitle = MakeLabel("Default White Atlas Brushes", FColor::FromBytes(255, 214, 102));
    whiteIconsTitle->SetFontSize(18.0f);
    root->AddChild(whiteIconsTitle, FMargin(20.0f, 12.0f, 20.0f, 0.0f));
    root->AddChild(MakeCoreIconGrid(*app, false), FMargin(20.0f, 0.0f, 20.0f, 0.0f));

    auto tintedIconsTitle = MakeLabel("Tinted Atlas Brushes", FColor::FromBytes(255, 214, 102));
    tintedIconsTitle->SetFontSize(18.0f);
    root->AddChild(tintedIconsTitle, FMargin(20.0f, 12.0f, 20.0f, 0.0f));
    root->AddChild(MakeCoreIconGrid(*app, true), FMargin(20.0f, 0.0f, 20.0f, 20.0f));

    app->SetRootWidget(root);
    backend->Run();
    backend->Shutdown();

    return 0;
}
