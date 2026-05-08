#pragma once

#include <imwidgetv4/core/CoreIcon.h>
#include <imwidgetv4/widgets/Image.h>
#include <cstdint>
#include <vector>

namespace ImWidgetV4::CoreIconInternal {

constexpr int AtlasWidth = 224;
constexpr int AtlasHeight = 192;
constexpr int IconSize = 32;

const std::vector<std::uint8_t>& GetAtlasPixels();
int GetIconCount();
FImageBrush MakeBrush(ImTextureID atlasTextureId, ECoreIcon icon);

} // namespace ImWidgetV4::CoreIconInternal
