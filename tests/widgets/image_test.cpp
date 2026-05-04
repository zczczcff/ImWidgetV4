#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imwidgetv4/widgets/Image.h>
#include <imgui.h>
#include <memory>
#include <vector>

using namespace ImWidgetV4;

namespace {

class FImGuiScope {
public:
    FImGuiScope()
    {
        if (ImGui::GetCurrentContext() == nullptr) {
            Context_ = ImGui::CreateContext();
            Owned_ = true;
            ImGuiIO& io = ImGui::GetIO();
            io.Fonts->AddFontDefault();
            io.Fonts->Build();
            io.DisplaySize = ImVec2(320.0f, 200.0f);
            io.DeltaTime = 1.0f / 60.0f;
        }

        ImGui::NewFrame();
    }

    ~FImGuiScope()
    {
        if (ImGui::GetCurrentContext() != nullptr) {
            ImGui::EndFrame();
        }
        if (Owned_ && Context_ != nullptr) {
            ImGui::DestroyContext(Context_);
        }
    }

private:
    ImGuiContext* Context_ = nullptr;
    bool Owned_ = false;
};

std::vector<std::uint8_t> MakeCheckerTexture(int width, int height)
{
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const bool bright = ((x / 2) + (y / 2)) % 2 == 0;
            const std::size_t offset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(width) + static_cast<std::size_t>(x)) * 4U;
            pixels[offset] = bright ? 255 : 46;
            pixels[offset + 1] = bright ? 160 : 88;
            pixels[offset + 2] = bright ? 72 : 214;
            pixels[offset + 3] = 255;
        }
    }
    return pixels;
}

class ImageTest : public ::testing::Test {
protected:
    FSnapshotImage Capture(ImApplication& application, const std::shared_ptr<ImImage>& image, const FVector2& size)
    {
        application.SetRootWidget(image);
        return application.CaptureSnapshot(
            [] (const FVector2& viewportSize) {
                FFrameContext frameContext;
                frameContext.FrameInfo.ViewportSize = viewportSize;
                frameContext.FrameInfo.DeltaTime = 1.0f / 60.0f;
                return frameContext;
            }(size),
            FSnapshotOptions {
                static_cast<int>(size.X),
                static_cast<int>(size.Y),
                FColor::FromBytes(10, 12, 16, 255)
            });
    }
};

} // namespace

TEST_F(ImageTest, DefaultStateUsesPlaceholderAndStableMinSize)
{
    FImGuiScope imguiScope;
    ImApplication application;
    auto image = std::make_shared<ImImage>();

    const FVector2 minSize = image->GetMinSize();
    EXPECT_FLOAT_EQ(minSize.X, 96.0f);
    EXPECT_FLOAT_EQ(minSize.Y, 72.0f);

    const FSnapshotImage snapshot = Capture(application, image, FVector2(160.0f, 120.0f));
    EXPECT_GT(FSnapshotRenderer::ComputeHash(snapshot), 0u);
}

TEST_F(ImageTest, NullTextureAndEmptyBrushShareTheFallbackImage)
{
    FImGuiScope imguiScope;
    ImApplication application;

    auto defaultImage = std::make_shared<ImImage>();
    const FSnapshotImage defaultSnapshot = Capture(application, defaultImage, FVector2(160.0f, 120.0f));

    auto nullTextureImage = std::make_shared<ImImage>();
    nullTextureImage->SetTexture(nullptr, FVector2(24.0f, 24.0f));
    const FSnapshotImage nullTextureSnapshot = Capture(application, nullTextureImage, FVector2(160.0f, 120.0f));

    auto emptyBrushImage = std::make_shared<ImImage>();
    emptyBrushImage->SetBrush(FImageBrush());
    const FSnapshotImage emptyBrushSnapshot = Capture(application, emptyBrushImage, FVector2(160.0f, 120.0f));

    EXPECT_EQ(FSnapshotRenderer::ComputeHash(defaultSnapshot), FSnapshotRenderer::ComputeHash(nullTextureSnapshot));
    EXPECT_EQ(FSnapshotRenderer::ComputeHash(defaultSnapshot), FSnapshotRenderer::ComputeHash(emptyBrushSnapshot));
}

TEST_F(ImageTest, ValidTextureUsesSourceSizeUntilDesiredSizeOverridesIt)
{
    FImGuiScope imguiScope;
    ImApplication application;
    auto image = std::make_shared<ImImage>();

    const ImTextureID textureId = application.CreateRuntimeTextureFromRgba(MakeCheckerTexture(8, 4), 8, 4);
    ASSERT_NE(textureId, nullptr);

    image->SetTexture(textureId, FVector2(8.0f, 4.0f));
    FVector2 minSize = image->GetMinSize();
    EXPECT_FLOAT_EQ(minSize.X, 8.0f);
    EXPECT_FLOAT_EQ(minSize.Y, 4.0f);

    image->SetDesiredSize(FVector2(48.0f, 20.0f));
    minSize = image->GetMinSize();
    EXPECT_FLOAT_EQ(minSize.X, 48.0f);
    EXPECT_FLOAT_EQ(minSize.Y, 20.0f);
}

TEST_F(ImageTest, StretchTintAndBackgroundAffectTheRenderedResult)
{
    FImGuiScope imguiScope;
    ImApplication application;
    const ImTextureID textureId = application.CreateRuntimeTextureFromRgba(MakeCheckerTexture(16, 16), 16, 16);
    ASSERT_NE(textureId, nullptr);

    auto keepAspectImage = std::make_shared<ImImage>();
    keepAspectImage->SetTexture(textureId, FVector2(16.0f, 16.0f));
    keepAspectImage->SetDesiredSize(FVector2(120.0f, 60.0f));
    const FSnapshotImage keepAspectSnapshot = Capture(application, keepAspectImage, FVector2(120.0f, 60.0f));

    auto fillImage = std::make_shared<ImImage>();
    fillImage->SetTexture(textureId, FVector2(16.0f, 16.0f));
    fillImage->SetDesiredSize(FVector2(120.0f, 60.0f));
    fillImage->SetStretchMode(EImageStretchMode::Fill);
    fillImage->SetTint(FColor::FromBytes(255, 220, 128));
    fillImage->SetBackgroundColor(FColor::FromBytes(24, 30, 38));
    fillImage->SetCornerRadius(14.0f);
    const FSnapshotImage fillSnapshot = Capture(application, fillImage, FVector2(120.0f, 60.0f));

    EXPECT_NE(FSnapshotRenderer::ComputeHash(keepAspectSnapshot), FSnapshotRenderer::ComputeHash(fillSnapshot));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
