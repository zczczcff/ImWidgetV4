#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/core/DrawContext.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imwidgetv4/widgets/Button.h>
#include <imwidgetv4/widgets/TextBlock.h>
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
