#include "CoreIconData.h"
#include <array>
#include <cstddef>

namespace ImWidgetV4::CoreIconInternal {
namespace {

constexpr int GAtlasColumns = 8;
constexpr int GAtlasRows = 9;
constexpr int GIconCount = 67;
constexpr int GMaskByteCount = (AtlasWidth * AtlasHeight) / 8;

const std::array<std::uint8_t, GMaskByteCount> GCoreIconMask = {
#include "CoreIconMask.inl"
};

std::vector<std::uint8_t> BuildAtlasPixels()
{
    std::vector<std::uint8_t> pixels(
        static_cast<std::size_t>(AtlasWidth) * static_cast<std::size_t>(AtlasHeight) * 4U,
        0U);

    const int pixelCount = AtlasWidth * AtlasHeight;
    for (int pixelIndex = 0; pixelIndex < pixelCount; ++pixelIndex) {
        const std::uint8_t maskByte = GCoreIconMask[static_cast<std::size_t>(pixelIndex / 8)];
        const bool bOpaque = ((maskByte >> (pixelIndex % 8)) & 0x01U) != 0U;
        if (!bOpaque) {
            continue;
        }

        const std::size_t offset = static_cast<std::size_t>(pixelIndex) * 4U;
        pixels[offset] = 255;
        pixels[offset + 1] = 255;
        pixels[offset + 2] = 255;
        pixels[offset + 3] = 255;
    }

    return pixels;
}

} // namespace

const std::vector<std::uint8_t>& GetAtlasPixels()
{
    static const std::vector<std::uint8_t> atlasPixels = BuildAtlasPixels();
    return atlasPixels;
}

int GetIconCount()
{
    return GIconCount;
}

FImageBrush MakeBrush(ImTextureID atlasTextureId, ECoreIcon icon)
{
    FImageBrush brush;
    const int iconIndex = static_cast<int>(icon);
    if (atlasTextureId == nullptr || iconIndex < 0 || iconIndex >= GIconCount) {
        return brush;
    }

    const int column = iconIndex % GAtlasColumns;
    const int row = iconIndex / GAtlasColumns;

    brush.TextureId = atlasTextureId;
    brush.SourceSize = FVector2(static_cast<float>(IconSize), static_cast<float>(IconSize));
    brush.Uv0 = FVector2(
        static_cast<float>(column * IconSize) / static_cast<float>(AtlasWidth),
        static_cast<float>(row * IconSize) / static_cast<float>(AtlasHeight));
    brush.Uv1 = FVector2(
        static_cast<float>((column + 1) * IconSize) / static_cast<float>(AtlasWidth),
        static_cast<float>((row + 1) * IconSize) / static_cast<float>(AtlasHeight));
    brush.TintColor = FColor::White;
    return brush;
}

} // namespace ImWidgetV4::CoreIconInternal
