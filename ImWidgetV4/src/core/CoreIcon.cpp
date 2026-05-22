#include <imwidgetv4/core/CoreIcon.h>

#include "CoreIconData.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>

namespace ImWidgetV4 {

namespace {

constexpr std::array<const char*, 94> GCoreIconNames = {{
    "Save",
    "Folder",
    "File",
    "Copy",
    "Paste",
    "Cut",
    "Trash",
    "Undo",
    "Redo",
    "Search",
    "Settings",
    "Add",
    "Remove",
    "ArrowUp",
    "ArrowDown",
    "Download",
    "Upload",
    "Lock",
    "Unlock",
    "View",
    "Check",
    "Close",
    "Favorite",
    "Heart",
    "Home",
    "Refresh",
    "Print",
    "Info",
    "Warning",
    "Play",
    "Pause",
    "Stop",
    "FastForward",
    "Rewind",
    "User",
    "Mail",
    "Cart",
    "ZoomIn",
    "ZoomOut",
    "AddToCart",
    "Bookmark",
    "ExpandableBox",
    "Button",
    "ColorPalette",
    "CheckBox",
    "ComboBox",
    "EditableText",
    "HorizontalBox",
    "Slider",
    "Image",
    "ListView",
    "PopupMenu",
    "ScrollBox",
    "HorizontalSplitter",
    "Switch",
    "TabView",
    "TextBlock",
    "OutlineView",
    "UserWidget",
    "VerticalBox",
    "VerticalSplitter",
    "CanvasPanel",
    "DesignerSurface",
    "BoxSlot",
    "Style",
    "TextList",
    "TextOutlineView",
    "Configure",
    "Generate",
    "Build",
    "BuildAll",
    "Clean",
    "Rebuild",
    "Debug",
    "Install",
    "Test",
    "CMakeFile",
    "TargetSelection",
    "BuildOutput",
    "OpenBuildDirectory",
    "CMakeCache",
    "AddSourceFile",
    "AddLibrary",
    "AddExecutable",
    "DependencyGraph",
    "FindPackage",
    "Properties",
    "ClearCache",
    "OpenTerminal",
    "CompileCurrentFile",
    "ProjectTree",
    "Package",
    "MemoryCheck",
    "ParallelBuild",
}};

std::string NormalizeIconName(const std::string& name)
{
    std::string normalized;
    normalized.reserve(name.size());
    for (char c : name) {
        const unsigned char ch = static_cast<unsigned char>(c);
        if (std::isalnum(ch) != 0) {
            normalized.push_back(static_cast<char>(std::tolower(ch)));
        }
    }
    return normalized;
}

std::uint8_t ColorChannelToByte(float value)
{
    return static_cast<std::uint8_t>(std::clamp(value, 0.0f, 1.0f) * 255.0f + 0.5f);
}

std::uint16_t ToU16(std::size_t value)
{
    return static_cast<std::uint16_t>(value & 0xffffU);
}

std::uint32_t ToU32(std::size_t value)
{
    return static_cast<std::uint32_t>(value & 0xffffffffU);
}

void AppendU16(std::vector<std::uint8_t>& bytes, std::uint16_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
}

void AppendU32(std::vector<std::uint8_t>& bytes, std::uint32_t value)
{
    bytes.push_back(static_cast<std::uint8_t>(value & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xffU));
    bytes.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xffU));
}

std::vector<std::uint8_t> BuildBmpIconImage(const std::vector<std::uint8_t>& rgbaPixels, int size)
{
    std::vector<std::uint8_t> bytes;
    const int xorStride = size * 4;
    const int andStride = ((size + 31) / 32) * 4;
    const int pixelBytes = xorStride * size;
    const int maskBytes = andStride * size;
    bytes.reserve(40 + static_cast<std::size_t>(pixelBytes + maskBytes));

    AppendU32(bytes, 40);
    AppendU32(bytes, static_cast<std::uint32_t>(size));
    AppendU32(bytes, static_cast<std::uint32_t>(size * 2));
    AppendU16(bytes, 1);
    AppendU16(bytes, 32);
    AppendU32(bytes, 0);
    AppendU32(bytes, static_cast<std::uint32_t>(pixelBytes + maskBytes));
    AppendU32(bytes, 0);
    AppendU32(bytes, 0);
    AppendU32(bytes, 0);
    AppendU32(bytes, 0);

    for (int y = size - 1; y >= 0; --y) {
        for (int x = 0; x < size; ++x) {
            const std::size_t offset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) +
                static_cast<std::size_t>(x)) * 4U;
            bytes.push_back(rgbaPixels[offset + 2]);
            bytes.push_back(rgbaPixels[offset + 1]);
            bytes.push_back(rgbaPixels[offset]);
            bytes.push_back(rgbaPixels[offset + 3]);
        }
    }

    const std::size_t maskOffset = bytes.size();
    bytes.resize(maskOffset + static_cast<std::size_t>(maskBytes), 0U);
    for (int y = size - 1; y >= 0; --y) {
        const int destinationRow = size - 1 - y;
        for (int x = 0; x < size; ++x) {
            const std::size_t rgbaOffset = (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) +
                static_cast<std::size_t>(x)) * 4U;
            if (rgbaPixels[rgbaOffset + 3] >= 128U) {
                continue;
            }

            const std::size_t maskByteOffset =
                maskOffset +
                static_cast<std::size_t>(destinationRow * andStride) +
                static_cast<std::size_t>(x / 8);
            bytes[maskByteOffset] |= static_cast<std::uint8_t>(0x80U >> (x % 8));
        }
    }

    return bytes;
}

} // namespace

const char* GetCoreIconName(ECoreIcon icon)
{
    const int iconIndex = static_cast<int>(icon);
    if (iconIndex < 0 || iconIndex >= static_cast<int>(GCoreIconNames.size())) {
        return "";
    }
    return GCoreIconNames[static_cast<std::size_t>(iconIndex)];
}

std::vector<std::string> GetCoreIconNames()
{
    std::vector<std::string> names;
    names.reserve(GCoreIconNames.size());
    for (const char* name : GCoreIconNames) {
        names.emplace_back(name);
    }
    return names;
}

bool TryParseCoreIconName(const std::string& name, ECoreIcon& outIcon)
{
    const std::string normalizedName = NormalizeIconName(name);
    for (std::size_t index = 0; index < GCoreIconNames.size(); ++index) {
        if (NormalizeIconName(GCoreIconNames[index]) == normalizedName) {
            outIcon = static_cast<ECoreIcon>(index);
            return true;
        }
    }
    return false;
}

bool BuildCoreIconRgba(
    ECoreIcon icon,
    int size,
    const FColor& tint,
    const FColor& background,
    std::vector<std::uint8_t>& outPixels)
{
    const int iconIndex = static_cast<int>(icon);
    if (size <= 0 || iconIndex < 0 || iconIndex >= CoreIconInternal::GetIconCount()) {
        outPixels.clear();
        return false;
    }

    const int column = iconIndex % (CoreIconInternal::AtlasWidth / CoreIconInternal::IconSize);
    const int row = iconIndex / (CoreIconInternal::AtlasWidth / CoreIconInternal::IconSize);
    const std::vector<std::uint8_t>& atlasPixels = CoreIconInternal::GetAtlasPixels();
    const std::uint8_t tintR = ColorChannelToByte(tint.R);
    const std::uint8_t tintG = ColorChannelToByte(tint.G);
    const std::uint8_t tintB = ColorChannelToByte(tint.B);
    const std::uint8_t tintA = ColorChannelToByte(tint.A);
    const std::uint8_t backgroundR = ColorChannelToByte(background.R);
    const std::uint8_t backgroundG = ColorChannelToByte(background.G);
    const std::uint8_t backgroundB = ColorChannelToByte(background.B);
    const std::uint8_t backgroundA = ColorChannelToByte(background.A);

    outPixels.assign(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4U, 0U);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const int sourceX = column * CoreIconInternal::IconSize +
                std::clamp((x * CoreIconInternal::IconSize) / size, 0, CoreIconInternal::IconSize - 1);
            const int sourceY = row * CoreIconInternal::IconSize +
                std::clamp((y * CoreIconInternal::IconSize) / size, 0, CoreIconInternal::IconSize - 1);
            const std::size_t sourceOffset =
                (static_cast<std::size_t>(sourceY) * static_cast<std::size_t>(CoreIconInternal::AtlasWidth) +
                 static_cast<std::size_t>(sourceX)) * 4U;
            const bool bGlyph = atlasPixels[sourceOffset + 3] > 0U;
            const std::size_t destinationOffset =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)) * 4U;
            outPixels[destinationOffset] = bGlyph ? tintR : backgroundR;
            outPixels[destinationOffset + 1] = bGlyph ? tintG : backgroundG;
            outPixels[destinationOffset + 2] = bGlyph ? tintB : backgroundB;
            outPixels[destinationOffset + 3] = bGlyph ? tintA : backgroundA;
        }
    }
    return true;
}

bool ExportCoreIconIco(
    ECoreIcon icon,
    const std::filesystem::path& outputPath,
    const FColor& tint,
    const FColor& background)
{
    return ExportCoreIconIco(icon, outputPath, {16, 32, 48, 256}, tint, background);
}

bool ExportCoreIconIco(
    ECoreIcon icon,
    const std::filesystem::path& outputPath,
    const std::vector<int>& sizes,
    const FColor& tint,
    const FColor& background)
{
    std::vector<std::vector<std::uint8_t>> images;
    std::vector<int> normalizedSizes;
    for (int size : sizes) {
        if (size <= 0 || size > 256) {
            continue;
        }
        if (std::find(normalizedSizes.begin(), normalizedSizes.end(), size) != normalizedSizes.end()) {
            continue;
        }

        std::vector<std::uint8_t> rgbaPixels;
        if (!BuildCoreIconRgba(icon, size, tint, background, rgbaPixels)) {
            return false;
        }
        normalizedSizes.push_back(size);
        images.push_back(BuildBmpIconImage(rgbaPixels, size));
    }

    if (images.empty()) {
        return false;
    }

    try {
        const std::filesystem::path parentPath = outputPath.parent_path();
        if (!parentPath.empty()) {
            std::filesystem::create_directories(parentPath);
        }

        std::vector<std::uint8_t> bytes;
        const std::size_t directorySize = 6U + images.size() * 16U;
        std::size_t imageOffset = directorySize;
        bytes.reserve(directorySize);
        AppendU16(bytes, 0);
        AppendU16(bytes, 1);
        AppendU16(bytes, ToU16(images.size()));
        for (std::size_t index = 0; index < images.size(); ++index) {
            const int size = normalizedSizes[index];
            bytes.push_back(size >= 256 ? 0U : static_cast<std::uint8_t>(size));
            bytes.push_back(size >= 256 ? 0U : static_cast<std::uint8_t>(size));
            bytes.push_back(0);
            bytes.push_back(0);
            AppendU16(bytes, 1);
            AppendU16(bytes, 32);
            AppendU32(bytes, ToU32(images[index].size()));
            AppendU32(bytes, ToU32(imageOffset));
            imageOffset += images[index].size();
        }
        for (const std::vector<std::uint8_t>& image : images) {
            bytes.insert(bytes.end(), image.begin(), image.end());
        }

        std::ofstream stream(outputPath, std::ios::binary | std::ios::trunc);
        if (!stream.is_open()) {
            return false;
        }
        stream.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
        stream.flush();
        return stream.good();
    } catch (...) {
        return false;
    }
}

} // namespace ImWidgetV4
