#include <gtest/gtest.h>
#include <imwidgetv4/core/Application.h>
#include <imwidgetv4/snapshot/Snapshot.h>
#include <imwidgetv4/widgets/Image.h>
#include <imgui.h>
#include <array>
#include <memory>
#include <set>
#include <string>
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

constexpr std::array<ECoreIcon, 67> GAllCoreIcons = {
    ECoreIcon::Save,
    ECoreIcon::Folder,
    ECoreIcon::File,
    ECoreIcon::Copy,
    ECoreIcon::Paste,
    ECoreIcon::Cut,
    ECoreIcon::Trash,
    ECoreIcon::Undo,
    ECoreIcon::Redo,
    ECoreIcon::Search,
    ECoreIcon::Settings,
    ECoreIcon::Add,
    ECoreIcon::Remove,
    ECoreIcon::ArrowUp,
    ECoreIcon::ArrowDown,
    ECoreIcon::Download,
    ECoreIcon::Upload,
    ECoreIcon::Lock,
    ECoreIcon::Unlock,
    ECoreIcon::View,
    ECoreIcon::Check,
    ECoreIcon::Close,
    ECoreIcon::Favorite,
    ECoreIcon::Heart,
    ECoreIcon::Home,
    ECoreIcon::Refresh,
    ECoreIcon::Print,
    ECoreIcon::Info,
    ECoreIcon::Warning,
    ECoreIcon::Play,
    ECoreIcon::Pause,
    ECoreIcon::Stop,
    ECoreIcon::FastForward,
    ECoreIcon::Rewind,
    ECoreIcon::User,
    ECoreIcon::Mail,
    ECoreIcon::Cart,
    ECoreIcon::ZoomIn,
    ECoreIcon::ZoomOut,
    ECoreIcon::AddToCart,
    ECoreIcon::Bookmark,
    ECoreIcon::ExpandableBox,
    ECoreIcon::Button,
    ECoreIcon::ColorPalette,
    ECoreIcon::CheckBox,
    ECoreIcon::ComboBox,
    ECoreIcon::EditableText,
    ECoreIcon::HorizontalBox,
    ECoreIcon::Slider,
    ECoreIcon::Image,
    ECoreIcon::ListView,
    ECoreIcon::PopupMenu,
    ECoreIcon::ScrollBox,
    ECoreIcon::HorizontalSplitter,
    ECoreIcon::Switch,
    ECoreIcon::TabView,
    ECoreIcon::TextBlock,
    ECoreIcon::OutlineView,
    ECoreIcon::UserWidget,
    ECoreIcon::VerticalBox,
    ECoreIcon::VerticalSplitter,
    ECoreIcon::CanvasPanel,
    ECoreIcon::DesignerSurface,
    ECoreIcon::BoxSlot,
    ECoreIcon::Style,
    ECoreIcon::TextList,
    ECoreIcon::TextOutlineView
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

TEST_F(ImageTest, CoreIconBrushesShareAtlasTextureAndExposeDistinctUvs)
{
    FImGuiScope imguiScope;
    ImApplication application;

    ImTextureID atlasTextureId = nullptr;
    std::set<std::string> uvKeys;
    for (ECoreIcon icon : GAllCoreIcons) {
        const FImageBrush brush = application.GetCoreIconBrush(icon);
        ASSERT_TRUE(brush.IsValid());
        EXPECT_FLOAT_EQ(brush.SourceSize.X, 32.0f);
        EXPECT_FLOAT_EQ(brush.SourceSize.Y, 32.0f);

        if (atlasTextureId == nullptr) {
            atlasTextureId = brush.TextureId;
        }

        EXPECT_EQ(brush.TextureId, atlasTextureId);
        uvKeys.insert(
            std::to_string(brush.Uv0.X) + ":" +
            std::to_string(brush.Uv0.Y) + ":" +
            std::to_string(brush.Uv1.X) + ":" +
            std::to_string(brush.Uv1.Y));
    }

    EXPECT_EQ(uvKeys.size(), GAllCoreIcons.size());

    const FImageBrush whiteBrush = application.GetCoreIconBrush(ECoreIcon::Save);
    const FImageBrush tintedBrush = application.GetCoreIconBrush(ECoreIcon::Save, FColor::FromBytes(255, 128, 96));
    EXPECT_EQ(tintedBrush.TextureId, whiteBrush.TextureId);
    EXPECT_EQ(tintedBrush.Uv0, whiteBrush.Uv0);
    EXPECT_EQ(tintedBrush.Uv1, whiteBrush.Uv1);
    EXPECT_NE(tintedBrush.TintColor.ToImU32(), whiteBrush.TintColor.ToImU32());
}

TEST_F(ImageTest, CoreIconAtlasProducesTransparentBackgroundAndOpaqueGlyphPixels)
{
    FImGuiScope imguiScope;
    ImApplication application;
    const FImageBrush saveBrush = application.GetCoreIconBrush(ECoreIcon::Save);
    ASSERT_TRUE(saveBrush.IsValid());

    ImApplication::FRuntimeTextureData textureData;
    ASSERT_TRUE(application.FindRuntimeTextureData(saveBrush.TextureId, textureData));
    ASSERT_EQ(textureData.Width, 256);
    ASSERT_EQ(textureData.Height, 288);
    ASSERT_EQ(textureData.BytesPerPixel, 4);

    const auto samplePixel = [&textureData](int x, int y) {
        const std::size_t offset =
            (static_cast<std::size_t>(y) * static_cast<std::size_t>(textureData.Width) + static_cast<std::size_t>(x)) * 4U;
        return std::array<std::uint8_t, 4> {
            textureData.Pixels[offset],
            textureData.Pixels[offset + 1],
            textureData.Pixels[offset + 2],
            textureData.Pixels[offset + 3]
        };
    };

    EXPECT_EQ(samplePixel(0, 0), (std::array<std::uint8_t, 4> {0, 0, 0, 0}));
    EXPECT_EQ(samplePixel(8, 8), (std::array<std::uint8_t, 4> {255, 255, 255, 255}));
}

int main(int argc, char** argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
