#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imwidgetv4/style/StyleResolvers.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/Image.h>
#include <imwidgetv4/widgets/ScrollBox.h>
#include <imwidgetv4/widgets/TextList.h>
#include <imwidgetv4/widgets/TextBlock.h>
#include <imwidgetv4/widgets/UserWidget.h>
#include <imwidgetv4/widgets/VerticalBox.h>
#include <algorithm>
#include <filesystem>
#include <memory>
#include <vector>
#include <imgui.h>

using namespace ImWidgetV4;

namespace {

class FImGuiScope {
public:
    FImGuiScope() {
        IMGUI_CHECKVERSION();
        Context_ = ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.Fonts->Build();
        io.DisplaySize = ImVec2(256.0f, 128.0f);
        io.DeltaTime = 1.0f / 60.0f;
    }

    ~FImGuiScope() {
        ImGui::DestroyContext(Context_);
    }

private:
    ImGuiContext* Context_ = nullptr;
};

class StyleColorWidget : public ImWidget {
public:
    virtual void Paint(const FPaintContext& paintContext) override {
        const FColor color = paintContext.StyleSet != nullptr
            ? paintContext.StyleSet->GetColor("Background", FColor::White)
            : FColor::White;
        paintContext.DrawContext_.DrawRectFilled(m_Geometry.GetMin(), m_Geometry.GetMax(), color);
    }
};

class ThemeAwareButtonWidget : public ImWidget {
public:
    ThemeAwareButtonWidget()
    {
        Button = std::make_shared<ImButton>();
        Button->SetText("Theme");
        AddChild(Button);
    }

    void Paint(const FPaintContext& paintContext) override
    {
        Button->SetGeometry(m_Geometry);
        Button->Paint(paintContext);
    }

    FVector2 GetMinSize() const override
    {
        return Button->GetMinSize();
    }

    std::shared_ptr<ImButton> Button;
};

FFrameContext MakeFrameContext(float width, float height, double currentTime = 0.0) {
    FFrameContext frameContext;
    frameContext.FrameInfo.ViewportPosition = FVector2(0.0f, 0.0f);
    frameContext.FrameInfo.ViewportSize = FVector2(width, height);
    frameContext.FrameInfo.DeltaTime = 1.0f / 60.0f;
    frameContext.FrameInfo.CurrentTime = currentTime;
    return frameContext;
}

bool ImageHasAnyPixelDifferentFrom(const FSnapshotImage& image, const FColor& color) {
    const std::uint8_t expectedR = static_cast<std::uint8_t>(std::clamp(color.R, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t expectedG = static_cast<std::uint8_t>(std::clamp(color.G, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t expectedB = static_cast<std::uint8_t>(std::clamp(color.B, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t expectedA = static_cast<std::uint8_t>(std::clamp(color.A, 0.0f, 1.0f) * 255.0f);

    for (std::size_t offset = 0; offset + 3 < image.Pixels.size(); offset += 4) {
        if (image.Pixels[offset] != expectedR ||
            image.Pixels[offset + 1] != expectedG ||
            image.Pixels[offset + 2] != expectedB ||
            image.Pixels[offset + 3] != expectedA) {
            return true;
        }
    }

    return false;
}

FInputEvent MakeMouseMove(float x, float y) {
    FInputEvent event;
    event.Type = EInputEventType::MouseMove;
    event.MousePosition = FVector2(x, y);
    event.Timestamp = 0.0;
    return event;
}

class SnapshotUserWidget : public ImUserWidget {
protected:
    Ptr RebuildWidget() override {
        auto root = std::make_shared<ImVerticalBox>();
        root->SetSpacing(6.0f);

        auto title = std::make_shared<ImTextBlock>();
        title->SetText("UserWidget");
        title->SetTextColor(FColor::White);
        root->AddChild(title);

        auto body = std::make_shared<ImTextBlock>();
        body->SetText("Composite content");
        body->SetTextColor(FColor::FromBytes(255, 214, 102));
        root->AddChild(body);

        return root;
    }
};

} // namespace

TEST(SnapshotTest, CaptureWithoutImGuiContextReturnsClearImage) {
    ImApplication application;
    FFrameContext frameContext = MakeFrameContext(0.0f, 0.0f);
    const FSnapshotOptions options {32, 16, FColor::FromBytes(12, 34, 56, 78)};

    const FSnapshotImage image = application.CaptureSnapshot(frameContext, options);

    ASSERT_EQ(image.Width, 32);
    ASSERT_EQ(image.Height, 16);
    ASSERT_EQ(image.Pixels.size(), static_cast<std::size_t>(32 * 16 * 4));

    for (std::size_t offset = 0; offset + 3 < image.Pixels.size(); offset += 4) {
        EXPECT_EQ(image.Pixels[offset], 12);
        EXPECT_EQ(image.Pixels[offset + 1], 34);
        EXPECT_EQ(image.Pixels[offset + 2], 56);
        EXPECT_EQ(image.Pixels[offset + 3], 78);
    }
}

TEST(SnapshotTest, ExportSnapshotToPngWritesFileWithoutImGuiContext) {
    ImApplication application;
    FFrameContext frameContext = MakeFrameContext(40.0f, 20.0f);
    const FSnapshotOptions options {40, 20, FColor::FromBytes(10, 12, 16, 255)};
    const std::filesystem::path outputPath =
        std::filesystem::temp_directory_path() / "imwidgetv4_snapshot_export_test.png";
    std::error_code errorCode;
    std::filesystem::remove(outputPath, errorCode);

    ASSERT_TRUE(application.ExportSnapshotToPng(outputPath, frameContext, options));
    ASSERT_TRUE(std::filesystem::exists(outputPath));
    EXPECT_GT(std::filesystem::file_size(outputPath), 0u);

    std::filesystem::remove(outputPath, errorCode);
}

TEST(SnapshotTest, CaptureSnapshotWithTextProducesNonBackgroundPixels) {
    FImGuiScope imguiScope;
    ImApplication application;
    auto text = std::make_shared<ImTextBlock>();
    text->SetText("Snapshot");
    text->SetTextColor(FColor::White);
    application.SetRootWidget(text);

    const FColor clearColor = FColor::FromBytes(10, 12, 16, 255);
    const FSnapshotImage image = application.CaptureSnapshot(
        MakeFrameContext(220.0f, 80.0f),
        FSnapshotOptions {220, 80, clearColor});

    ASSERT_EQ(image.Width, 220);
    ASSERT_EQ(image.Height, 80);
    EXPECT_TRUE(ImageHasAnyPixelDifferentFrom(image, clearColor));
}

TEST(SnapshotTest, PlaceholderImageSnapshotProducesNonBackgroundPixels) {
    FImGuiScope imguiScope;
    ImApplication application;
    auto image = std::make_shared<ImImage>();
    application.SetRootWidget(image);

    const FColor clearColor = FColor::FromBytes(10, 12, 16, 255);
    const FSnapshotImage snapshot = application.CaptureSnapshot(
        MakeFrameContext(160.0f, 120.0f),
        FSnapshotOptions {160, 120, clearColor});

    EXPECT_TRUE(ImageHasAnyPixelDifferentFrom(snapshot, clearColor));
}

TEST(SnapshotTest, SnapshotHashAndCompareAreDeterministicForSameTree) {
    FImGuiScope imguiScope;
    ImApplication application;
    auto text = std::make_shared<ImTextBlock>();
    text->SetText("Deterministic");
    text->SetTextColor(FColor::White);
    application.SetRootWidget(text);

    const FSnapshotOptions options {240, 90, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage first = application.CaptureSnapshot(MakeFrameContext(240.0f, 90.0f, 0.0), options);
    const FSnapshotImage second = application.CaptureSnapshot(MakeFrameContext(240.0f, 90.0f, 0.1), options);

    EXPECT_EQ(FSnapshotRenderer::ComputeHash(first), FSnapshotRenderer::ComputeHash(second));
    const FSnapshotComparison comparison = FSnapshotRenderer::Compare(first, second, 0);
    EXPECT_TRUE(comparison.IsMatch());
}

TEST(SnapshotTest, ButtonSnapshotsDifferAcrossHoverState) {
    FImGuiScope imguiScope;
    ImApplication application;
    auto button = std::make_shared<ImButton>();
    button->SetText("Hover Me");
    application.SetRootWidget(button);

    const FFrameContext frameContext = MakeFrameContext(140.0f, 40.0f, 0.0);
    const FSnapshotOptions options {140, 40, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage normal = application.CaptureSnapshot(frameContext, options);

    std::vector<FInputEvent> inputEvents {MakeMouseMove(30.0f, 20.0f)};
    FFrameContext hoverContext = frameContext;
    hoverContext.InputEvents = &inputEvents;
    application.AdvanceFrame(hoverContext);

    const FSnapshotImage hovered = application.CaptureSnapshot(frameContext, options);
    EXPECT_NE(FSnapshotRenderer::ComputeHash(normal), FSnapshotRenderer::ComputeHash(hovered));
    EXPECT_FALSE(FSnapshotRenderer::Compare(normal, hovered, 0).IsMatch());
}

TEST(SnapshotTest, ThemeSwitchChangesSnapshotWithoutRebuildingRoot) {
    FImGuiScope imguiScope;
    ImApplication application;
    auto widget = std::make_shared<StyleColorWidget>();
    application.SetRootWidget(widget);

    const FSnapshotOptions options {120, 60, FColor::Transparent};
    const FSnapshotImage defaultTheme = application.CaptureSnapshot(MakeFrameContext(120.0f, 60.0f), options);
    ASSERT_TRUE(application.SetActiveTheme("Dark"));
    const FSnapshotImage darkTheme = application.CaptureSnapshot(MakeFrameContext(120.0f, 60.0f), options);

    EXPECT_NE(FSnapshotRenderer::ComputeHash(defaultTheme), FSnapshotRenderer::ComputeHash(darkTheme));
    EXPECT_FALSE(FSnapshotRenderer::Compare(defaultTheme, darkTheme, 0).IsMatch());
}

TEST(SnapshotTest, ThemeSwitchUpdatesThemeAwareButtonWithoutExplicitStyle)
{
    FImGuiScope imguiScope;
    ImApplication application;
    auto widget = std::make_shared<ThemeAwareButtonWidget>();
    application.SetRootWidget(widget);

    const FSnapshotOptions options {160, 60, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage defaultTheme = application.CaptureSnapshot(MakeFrameContext(160.0f, 60.0f), options);
    ASSERT_TRUE(application.SetActiveTheme("Dark"));
    const FSnapshotImage darkTheme = application.CaptureSnapshot(MakeFrameContext(160.0f, 60.0f), options);

    EXPECT_NE(FSnapshotRenderer::ComputeHash(defaultTheme), FSnapshotRenderer::ComputeHash(darkTheme));
}

TEST(SnapshotTest, MultiWindowSnapshotChangesWhenModalAppears) {
    FImGuiScope imguiScope;
    ImApplication application;

    auto mainText = std::make_shared<ImTextBlock>();
    mainText->SetText("Main Content");
    mainText->SetTextColor(FColor::White);
    application.SetRootWidget(mainText);

    FWindowOptions toolsOptions;
    toolsOptions.Title = "Tools";
    toolsOptions.Position = FVector2(28.0f, 18.0f);
    toolsOptions.Size = FVector2(140.0f, 84.0f);
    auto toolsText = std::make_shared<ImTextBlock>();
    toolsText->SetText("Floating");
    toolsText->SetTextColor(FColor::FromBytes(255, 214, 102));
    toolsOptions.RootWidget = toolsText;
    application.GetWindowManager().CreateWindow(toolsOptions);

    const FFrameContext frameContext = MakeFrameContext(240.0f, 140.0f, 0.0);
    const FSnapshotOptions options {240, 140, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage withoutModal = application.CaptureSnapshot(frameContext, options);

    FPopupOptions modalOptions;
    modalOptions.Title = "Confirm";
    modalOptions.Position = FVector2(70.0f, 42.0f);
    modalOptions.Size = FVector2(120.0f, 70.0f);
    auto modalText = std::make_shared<ImTextBlock>();
    modalText->SetText("Modal");
    modalText->SetTextColor(FColor::White);
    modalOptions.RootWidget = modalText;
    application.GetWindowManager().CreateModal(modalOptions);

    const FSnapshotImage withModal = application.CaptureSnapshot(frameContext, options);
    EXPECT_NE(FSnapshotRenderer::ComputeHash(withoutModal), FSnapshotRenderer::ComputeHash(withModal));
    EXPECT_FALSE(FSnapshotRenderer::Compare(withoutModal, withModal, 0).IsMatch());
}

TEST(SnapshotTest, ScrollBoxSnapshotChangesAfterWheelScroll) {
    FImGuiScope imguiScope;
    ImApplication application;

    auto list = std::make_shared<ImVerticalBox>();
    list->SetSpacing(6.0f);
    for (int index = 0; index < 18; ++index) {
        auto row = std::make_shared<ImTextBlock>();
        row->SetText("Scrollable Row " + std::to_string(index));
        row->SetTextColor(index % 2 == 0 ? FColor::White : FColor::FromBytes(255, 214, 102));
        list->AddChild(row, FMargin(12.0f, 12.0f, 6.0f, 0.0f));
    }

    auto scrollBox = std::make_shared<ImScrollBox>();
    scrollBox->SetContent(list);
    application.SetRootWidget(scrollBox);

    const FFrameContext frameContext = MakeFrameContext(220.0f, 140.0f, 0.0);
    const FSnapshotOptions options {220, 140, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage beforeScroll = application.CaptureSnapshot(frameContext, options);

    FInputEvent wheelEvent;
    wheelEvent.Type = EInputEventType::MouseWheel;
    wheelEvent.MousePosition = FVector2(110.0f, 70.0f);
    wheelEvent.ScrollDelta = FVector2(0.0f, -4.0f);
    std::vector<FInputEvent> inputEvents {wheelEvent};

    FFrameContext wheelContext = frameContext;
    wheelContext.InputEvents = &inputEvents;
    application.AdvanceFrame(wheelContext);

    const FSnapshotImage afterScroll = application.CaptureSnapshot(frameContext, options);
    EXPECT_NE(FSnapshotRenderer::ComputeHash(beforeScroll), FSnapshotRenderer::ComputeHash(afterScroll));
    EXPECT_FALSE(FSnapshotRenderer::Compare(beforeScroll, afterScroll, 0).IsMatch());
}

TEST(SnapshotTest, ImageSnapshotChangesWhenRuntimeTextureReplacesPlaceholder) {
    FImGuiScope imguiScope;
    ImApplication application;

    auto image = std::make_shared<ImImage>();
    image->SetDesiredSize(FVector2(140.0f, 90.0f));
    application.SetRootWidget(image);

    const FFrameContext frameContext = MakeFrameContext(160.0f, 120.0f, 0.0);
    const FSnapshotOptions options {160, 120, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage placeholder = application.CaptureSnapshot(frameContext, options);

    std::vector<std::uint8_t> pixels(12U * 8U * 4U, 0U);
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < 12; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * 12U + static_cast<std::size_t>(x)) * 4U;
            pixels[offset] = static_cast<std::uint8_t>(32 + x * 16);
            pixels[offset + 1] = static_cast<std::uint8_t>(64 + y * 20);
            pixels[offset + 2] = 220;
            pixels[offset + 3] = 255;
        }
    }

    const ImTextureID textureId = application.CreateRuntimeTextureFromRgba(pixels, 12, 8);
    ASSERT_NE(textureId, nullptr);
    image->SetTexture(textureId, FVector2(12.0f, 8.0f));

    const FSnapshotImage textured = application.CaptureSnapshot(frameContext, options);
    EXPECT_NE(FSnapshotRenderer::ComputeHash(placeholder), FSnapshotRenderer::ComputeHash(textured));
    EXPECT_FALSE(FSnapshotRenderer::Compare(placeholder, textured, 0).IsMatch());

    const FSnapshotImage texturedAgain = application.CaptureSnapshot(frameContext, options);
    EXPECT_EQ(FSnapshotRenderer::ComputeHash(textured), FSnapshotRenderer::ComputeHash(texturedAgain));
}

TEST(SnapshotTest, CoreIconSnapshotIsVisibleAndDeterministic) {
    FImGuiScope imguiScope;
    ImApplication application;

    auto image = std::make_shared<ImImage>();
    image->SetBrush(application.GetCoreIconBrush(ECoreIcon::Search, FColor::FromBytes(255, 214, 102)));
    image->SetDesiredSize(FVector2(96.0f, 96.0f));
    image->SetBackgroundColor(FColor::FromBytes(20, 24, 30));
    application.SetRootWidget(image);

    const FColor clearColor = FColor::FromBytes(8, 10, 14, 255);
    const FFrameContext frameContext = MakeFrameContext(128.0f, 128.0f, 0.0);
    const FSnapshotOptions options {128, 128, clearColor};
    const FSnapshotImage first = application.CaptureSnapshot(frameContext, options);
    const FSnapshotImage second = application.CaptureSnapshot(frameContext, options);

    EXPECT_TRUE(ImageHasAnyPixelDifferentFrom(first, clearColor));
    EXPECT_EQ(FSnapshotRenderer::ComputeHash(first), FSnapshotRenderer::ComputeHash(second));
    EXPECT_TRUE(FSnapshotRenderer::Compare(first, second, 0).IsMatch());
}

TEST(SnapshotTest, UserWidgetSnapshotCapturesInternalRootTree) {
    FImGuiScope imguiScope;
    ImApplication application;

    auto widget = std::make_shared<SnapshotUserWidget>();
    application.SetRootWidget(widget);

    const FColor clearColor = FColor::FromBytes(8, 10, 14, 255);
    const FSnapshotImage snapshot = application.CaptureSnapshot(
        MakeFrameContext(220.0f, 100.0f, 0.0),
        FSnapshotOptions {220, 100, clearColor});

    EXPECT_TRUE(ImageHasAnyPixelDifferentFrom(snapshot, clearColor));
}

TEST(SnapshotTest, TextListSnapshotChangesAfterTextSelection) {
    FImGuiScope imguiScope;
    ImApplication application;

    auto list = std::make_shared<ImTextList>();
    list->SetItems({
        "A wrapped entry with enough text to make the control visually distinct even before any interaction happens.",
        "A second entry that can be partially selected through routed input.",
        "A third entry that keeps the viewport looking like a real log list."
    });
    application.SetRootWidget(list);

    const FFrameContext frameContext = MakeFrameContext(260.0f, 140.0f, 0.0);
    const FSnapshotOptions options {260, 140, FColor::FromBytes(8, 10, 14, 255)};
    const FSnapshotImage normal = application.CaptureSnapshot(frameContext, options);

    std::vector<FInputEvent> inputEvents {
        [] {
            FInputEvent event;
            event.Type = EInputEventType::MouseButtonDown;
            event.MousePosition = FVector2(40.0f, 70.0f);
            event.MouseButton = EMouseButton::Left;
            return event;
        }(),
        [] {
            FInputEvent event;
            event.Type = EInputEventType::MouseMove;
            event.MousePosition = FVector2(170.0f, 96.0f);
            event.MouseButton = EMouseButton::Left;
            return event;
        }(),
        [] {
            FInputEvent event;
            event.Type = EInputEventType::MouseButtonUp;
            event.MousePosition = FVector2(170.0f, 96.0f);
            event.MouseButton = EMouseButton::Left;
            return event;
        }()
    };

    FFrameContext inputContext = frameContext;
    inputContext.InputEvents = &inputEvents;
    application.AdvanceFrame(inputContext);

    const FSnapshotImage selected = application.CaptureSnapshot(frameContext, options);
    EXPECT_NE(FSnapshotRenderer::ComputeHash(normal), FSnapshotRenderer::ComputeHash(selected));
    EXPECT_FALSE(FSnapshotRenderer::Compare(normal, selected, 0).IsMatch());
}

TEST(SnapshotTest, CompareReportsDimensionAndToleranceDifferences) {
    FSnapshotImage expected;
    expected.Reset(2, 1);
    expected.Pixels = {
        0, 0, 0, 255,
        100, 100, 100, 255
    };

    FSnapshotImage actual;
    actual.Reset(3, 1);
    actual.Pixels = {
        0, 0, 0, 255,
        102, 100, 100, 255,
        255, 255, 255, 255
    };

    const FSnapshotComparison strictComparison = FSnapshotRenderer::Compare(expected, actual, 0);
    EXPECT_EQ(strictComparison.ExpectedWidth, 2);
    EXPECT_EQ(strictComparison.ActualWidth, 3);
    EXPECT_FALSE(strictComparison.IsMatch());
    EXPECT_GT(strictComparison.DifferentPixelCount, 0u);
    EXPECT_GE(strictComparison.MaxChannelDelta, 2);

    const FSnapshotComparison tolerantComparison = FSnapshotRenderer::Compare(expected, actual, 2);
    EXPECT_FALSE(tolerantComparison.IsMatch());
    EXPECT_LT(tolerantComparison.DifferentPixelCount, strictComparison.DifferentPixelCount);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
