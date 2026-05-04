#include <imwidgetv4/snapshot/Snapshot.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <filesystem>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace ImWidgetV4 {
namespace {

struct FTextureSampleSource {
    const std::uint8_t* Pixels = nullptr;
    int Width = 0;
    int Height = 0;
    int BytesPerPixel = 0;
    bool bAlphaOnly = false;
};

struct FVertexColor {
    float R = 1.0f;
    float G = 1.0f;
    float B = 1.0f;
    float A = 1.0f;
};

std::array<std::uint8_t, 4> DecodeColor(ImU32 packedColor) {
    return {
        static_cast<std::uint8_t>(packedColor & 0xffU),
        static_cast<std::uint8_t>((packedColor >> 8U) & 0xffU),
        static_cast<std::uint8_t>((packedColor >> 16U) & 0xffU),
        static_cast<std::uint8_t>((packedColor >> 24U) & 0xffU)
    };
}

void ClearImage(FSnapshotImage& image, const FColor& clearColor) {
    const std::uint8_t r = static_cast<std::uint8_t>(std::clamp(clearColor.R, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t g = static_cast<std::uint8_t>(std::clamp(clearColor.G, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t b = static_cast<std::uint8_t>(std::clamp(clearColor.B, 0.0f, 1.0f) * 255.0f);
    const std::uint8_t a = static_cast<std::uint8_t>(std::clamp(clearColor.A, 0.0f, 1.0f) * 255.0f);

    for (int index = 0; index < image.Width * image.Height; ++index) {
        const std::size_t offset = static_cast<std::size_t>(index) * 4U;
        image.Pixels[offset] = r;
        image.Pixels[offset + 1] = g;
        image.Pixels[offset + 2] = b;
        image.Pixels[offset + 3] = a;
    }
}

float EdgeFunction(const ImVec2& a, const ImVec2& b, const ImVec2& c) {
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

FTextureSampleSource ResolveTextureSource(ImTextureID textureId) {
    FTextureSampleSource source;
    if (ImGui::GetCurrentContext() == nullptr) {
        return source;
    }

    ImFontAtlas* fontAtlas = ImGui::GetIO().Fonts;
    if (fontAtlas == nullptr || textureId != fontAtlas->TexID) {
        return source;
    }

    int width = 0;
    int height = 0;
    int bytesPerPixel = 0;

    if (fontAtlas->TexPixelsRGBA32 != nullptr) {
        source.Pixels = reinterpret_cast<const std::uint8_t*>(fontAtlas->TexPixelsRGBA32);
        source.Width = fontAtlas->TexWidth;
        source.Height = fontAtlas->TexHeight;
        source.BytesPerPixel = 4;
        source.bAlphaOnly = false;
        return source;
    }

    if (fontAtlas->TexPixelsAlpha8 != nullptr) {
        source.Pixels = fontAtlas->TexPixelsAlpha8;
        source.Width = fontAtlas->TexWidth;
        source.Height = fontAtlas->TexHeight;
        source.BytesPerPixel = 1;
        source.bAlphaOnly = true;
        return source;
    }

    unsigned char* pixels = nullptr;
    fontAtlas->GetTexDataAsRGBA32(&pixels, &width, &height, &bytesPerPixel);
    source.Pixels = pixels;
    source.Width = width;
    source.Height = height;
    source.BytesPerPixel = bytesPerPixel;
    source.bAlphaOnly = bytesPerPixel == 1;
    return source;
}

FVertexColor SampleTextureColor(const FTextureSampleSource& source, const ImVec2& uv) {
    if (source.Pixels == nullptr || source.Width <= 0 || source.Height <= 0 || source.BytesPerPixel <= 0) {
        return {};
    }

    const float clampedU = std::clamp(uv.x, 0.0f, 1.0f);
    const float clampedV = std::clamp(uv.y, 0.0f, 1.0f);
    const int sampleX = std::clamp(
        static_cast<int>(std::floor(clampedU * static_cast<float>(source.Width))),
        0,
        source.Width - 1);
    const int sampleY = std::clamp(
        static_cast<int>(std::floor(clampedV * static_cast<float>(source.Height))),
        0,
        source.Height - 1);
    const std::size_t offset =
        (static_cast<std::size_t>(sampleY) * static_cast<std::size_t>(source.Width) + static_cast<std::size_t>(sampleX)) *
        static_cast<std::size_t>(source.BytesPerPixel);

    if (source.bAlphaOnly || source.BytesPerPixel == 1) {
        const float alpha = static_cast<float>(source.Pixels[offset]) / 255.0f;
        return FVertexColor {1.0f, 1.0f, 1.0f, alpha};
    }

    if (source.BytesPerPixel >= 4) {
        return FVertexColor {
            static_cast<float>(source.Pixels[offset]) / 255.0f,
            static_cast<float>(source.Pixels[offset + 1]) / 255.0f,
            static_cast<float>(source.Pixels[offset + 2]) / 255.0f,
            static_cast<float>(source.Pixels[offset + 3]) / 255.0f
        };
    }

    return {};
}

void BlendPixel(FSnapshotImage& image, int x, int y, const FVertexColor& sourceColor) {
    if (x < 0 || y < 0 || x >= image.Width || y >= image.Height) {
        return;
    }

    const std::size_t offset =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.Width) + static_cast<std::size_t>(x)) * 4U;

    const float srcA = std::clamp(sourceColor.A, 0.0f, 1.0f);
    const float dstA = static_cast<float>(image.Pixels[offset + 3]) / 255.0f;
    const float outA = srcA + dstA * (1.0f - srcA);

    auto BlendChannel = [&](std::size_t channel, float srcValue) {
        const float dstValue = static_cast<float>(image.Pixels[offset + channel]) / 255.0f;
        const float outValue = outA > 0.0f ? (srcValue * srcA + dstValue * dstA * (1.0f - srcA)) / outA : 0.0f;
        image.Pixels[offset + channel] =
            static_cast<std::uint8_t>(std::clamp(outValue, 0.0f, 1.0f) * 255.0f);
    };

    BlendChannel(0, sourceColor.R);
    BlendChannel(1, sourceColor.G);
    BlendChannel(2, sourceColor.B);
    image.Pixels[offset + 3] = static_cast<std::uint8_t>(std::clamp(outA, 0.0f, 1.0f) * 255.0f);
}

void RasterizeTriangle(
    FSnapshotImage& image,
    const ImDrawVert& v0,
    const ImDrawVert& v1,
    const ImDrawVert& v2,
    const FTextureSampleSource& textureSource,
    const ImVec4& clipRect,
    float scaleX,
    float scaleY,
    const ImVec2& displayPos) {
    const ImVec2 p0((v0.pos.x - displayPos.x) * scaleX, (v0.pos.y - displayPos.y) * scaleY);
    const ImVec2 p1((v1.pos.x - displayPos.x) * scaleX, (v1.pos.y - displayPos.y) * scaleY);
    const ImVec2 p2((v2.pos.x - displayPos.x) * scaleX, (v2.pos.y - displayPos.y) * scaleY);

    const float area = EdgeFunction(p0, p1, p2);
    if (std::fabs(area) < 0.0001f) {
        return;
    }

    const float clipMinX = (clipRect.x - displayPos.x) * scaleX;
    const float clipMinY = (clipRect.y - displayPos.y) * scaleY;
    const float clipMaxX = (clipRect.z - displayPos.x) * scaleX;
    const float clipMaxY = (clipRect.w - displayPos.y) * scaleY;
    const int minX = std::max(0, static_cast<int>(std::floor(std::max(std::min({p0.x, p1.x, p2.x}), clipMinX))));
    const int minY = std::max(0, static_cast<int>(std::floor(std::max(std::min({p0.y, p1.y, p2.y}), clipMinY))));
    const int maxX = std::min(
        image.Width - 1,
        static_cast<int>(std::ceil(std::min(std::max({p0.x, p1.x, p2.x}), clipMaxX))));
    const int maxY = std::min(
        image.Height - 1,
        static_cast<int>(std::ceil(std::min(std::max({p0.y, p1.y, p2.y}), clipMaxY))));

    if (minX > maxX || minY > maxY) {
        return;
    }

    const auto c0 = DecodeColor(v0.col);
    const auto c1 = DecodeColor(v1.col);
    const auto c2 = DecodeColor(v2.col);

    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const ImVec2 point(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);
            const float w0 = EdgeFunction(p1, p2, point);
            const float w1 = EdgeFunction(p2, p0, point);
            const float w2 = EdgeFunction(p0, p1, point);

            const bool sameWinding =
                (area >= 0.0f && w0 >= 0.0f && w1 >= 0.0f && w2 >= 0.0f) ||
                (area < 0.0f && w0 <= 0.0f && w1 <= 0.0f && w2 <= 0.0f);
            if (!sameWinding) {
                continue;
            }

            const float normalizedW0 = w0 / area;
            const float normalizedW1 = w1 / area;
            const float normalizedW2 = w2 / area;
            const ImVec2 uv(
                v0.uv.x * normalizedW0 + v1.uv.x * normalizedW1 + v2.uv.x * normalizedW2,
                v0.uv.y * normalizedW0 + v1.uv.y * normalizedW1 + v2.uv.y * normalizedW2);
            const FVertexColor textureColor = SampleTextureColor(textureSource, uv);

            const FVertexColor color {
                ((static_cast<float>(c0[0]) * normalizedW0 + static_cast<float>(c1[0]) * normalizedW1 +
                  static_cast<float>(c2[0]) * normalizedW2) /
                 255.0f) * textureColor.R,
                ((static_cast<float>(c0[1]) * normalizedW0 + static_cast<float>(c1[1]) * normalizedW1 +
                  static_cast<float>(c2[1]) * normalizedW2) /
                 255.0f) * textureColor.G,
                ((static_cast<float>(c0[2]) * normalizedW0 + static_cast<float>(c1[2]) * normalizedW1 +
                  static_cast<float>(c2[2]) * normalizedW2) /
                 255.0f) * textureColor.B,
                ((static_cast<float>(c0[3]) * normalizedW0 + static_cast<float>(c1[3]) * normalizedW1 +
                  static_cast<float>(c2[3]) * normalizedW2) /
                 255.0f) * textureColor.A
            };

            BlendPixel(image, x, y, color);
        }
    }
}

const std::uint8_t* GetPixelOrNull(const FSnapshotImage& image, int x, int y) {
    if (x < 0 || y < 0 || x >= image.Width || y >= image.Height || image.Pixels.empty()) {
        return nullptr;
    }

    const std::size_t offset =
        (static_cast<std::size_t>(y) * static_cast<std::size_t>(image.Width) + static_cast<std::size_t>(x)) * 4U;
    return image.Pixels.data() + offset;
}

} // namespace

void FSnapshotImage::Reset(int width, int height) {
    Width = std::max(0, width);
    Height = std::max(0, height);
    Pixels.assign(static_cast<std::size_t>(Width) * static_cast<std::size_t>(Height) * 4U, 0U);
}

bool FSnapshotComparison::IsMatch() const {
    return ExpectedWidth == ActualWidth &&
           ExpectedHeight == ActualHeight &&
           DifferentPixelCount == 0 &&
           MaxChannelDelta == 0;
}

FSnapshotImage FSnapshotRenderer::Rasterize(const ImDrawData& drawData, const FSnapshotOptions& options) {
    FSnapshotImage image;
    image.Reset(std::max(1, options.Width), std::max(1, options.Height));
    ClearImage(image, options.ClearColor);

    if (drawData.CmdListsCount <= 0 || drawData.DisplaySize.x <= 0.0f || drawData.DisplaySize.y <= 0.0f) {
        return image;
    }

    const float scaleX = static_cast<float>(image.Width) / drawData.DisplaySize.x;
    const float scaleY = static_cast<float>(image.Height) / drawData.DisplaySize.y;
    const ImVec2 displayPos = drawData.DisplayPos;

    for (int listIndex = 0; listIndex < drawData.CmdListsCount; ++listIndex) {
        const ImDrawList* drawList = drawData.CmdLists[listIndex];
        if (drawList == nullptr) {
            continue;
        }

        for (int cmdIndex = 0; cmdIndex < drawList->CmdBuffer.Size; ++cmdIndex) {
            const ImDrawCmd& drawCmd = drawList->CmdBuffer[cmdIndex];
            if (drawCmd.UserCallback != nullptr) {
                continue;
            }

            const FTextureSampleSource textureSource = ResolveTextureSource(drawCmd.TextureId);
            const unsigned int indexStart = drawCmd.IdxOffset;
            const unsigned int indexEnd = drawCmd.IdxOffset + drawCmd.ElemCount;

            for (unsigned int elemIndex = indexStart; elemIndex + 2 < indexEnd; elemIndex += 3) {
                const ImDrawIdx i0 = drawList->IdxBuffer[static_cast<int>(elemIndex)];
                const ImDrawIdx i1 = drawList->IdxBuffer[static_cast<int>(elemIndex + 1)];
                const ImDrawIdx i2 = drawList->IdxBuffer[static_cast<int>(elemIndex + 2)];

                const ImDrawVert& v0 = drawList->VtxBuffer[drawCmd.VtxOffset + i0];
                const ImDrawVert& v1 = drawList->VtxBuffer[drawCmd.VtxOffset + i1];
                const ImDrawVert& v2 = drawList->VtxBuffer[drawCmd.VtxOffset + i2];

                RasterizeTriangle(
                    image,
                    v0,
                    v1,
                    v2,
                    textureSource,
                    drawCmd.ClipRect,
                    scaleX,
                    scaleY,
                    displayPos);
            }
        }
    }

    return image;
}

bool FSnapshotRenderer::SavePng(const std::filesystem::path& filePath, const FSnapshotImage& image) {
    if (image.Width <= 0 || image.Height <= 0 || image.Pixels.empty()) {
        return false;
    }

    std::error_code errorCode;
    const std::filesystem::path parentPath = filePath.parent_path();
    if (!parentPath.empty()) {
        std::filesystem::create_directories(parentPath, errorCode);
        if (errorCode) {
            return false;
        }
    }

    return stbi_write_png(
               filePath.string().c_str(),
               image.Width,
               image.Height,
               4,
               image.Pixels.data(),
               image.Width * 4) != 0;
}

std::uint64_t FSnapshotRenderer::ComputeHash(const FSnapshotImage& image) {
    std::uint64_t hash = 1469598103934665603ULL;
    const auto MixByte = [&hash](std::uint8_t byte) {
        hash ^= static_cast<std::uint64_t>(byte);
        hash *= 1099511628211ULL;
    };

    const std::uint32_t width = static_cast<std::uint32_t>(std::max(0, image.Width));
    const std::uint32_t height = static_cast<std::uint32_t>(std::max(0, image.Height));
    for (int shift = 0; shift < 32; shift += 8) {
        MixByte(static_cast<std::uint8_t>((width >> shift) & 0xffU));
        MixByte(static_cast<std::uint8_t>((height >> shift) & 0xffU));
    }

    for (std::uint8_t byte : image.Pixels) {
        MixByte(byte);
    }

    return hash;
}

FSnapshotComparison FSnapshotRenderer::Compare(
    const FSnapshotImage& expected,
    const FSnapshotImage& actual,
    std::uint8_t channelTolerance) {
    FSnapshotComparison comparison;
    comparison.ExpectedWidth = expected.Width;
    comparison.ExpectedHeight = expected.Height;
    comparison.ActualWidth = actual.Width;
    comparison.ActualHeight = actual.Height;

    const int maxWidth = std::max(expected.Width, actual.Width);
    const int maxHeight = std::max(expected.Height, actual.Height);

    for (int y = 0; y < maxHeight; ++y) {
        for (int x = 0; x < maxWidth; ++x) {
            const std::uint8_t* expectedPixel = GetPixelOrNull(expected, x, y);
            const std::uint8_t* actualPixel = GetPixelOrNull(actual, x, y);

            bool bDifferentPixel = false;
            for (int channel = 0; channel < 4; ++channel) {
                const std::uint8_t expectedValue = expectedPixel != nullptr ? expectedPixel[channel] : 0U;
                const std::uint8_t actualValue = actualPixel != nullptr ? actualPixel[channel] : 0U;
                const std::uint8_t delta = static_cast<std::uint8_t>(
                    std::abs(static_cast<int>(expectedValue) - static_cast<int>(actualValue)));

                comparison.MaxChannelDelta = std::max(comparison.MaxChannelDelta, delta);
                if (delta > channelTolerance) {
                    bDifferentPixel = true;
                }
            }

            if (bDifferentPixel) {
                ++comparison.DifferentPixelCount;
            }
        }
    }

    return comparison;
}

} // namespace ImWidgetV4
