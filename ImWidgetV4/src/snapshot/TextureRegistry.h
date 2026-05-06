#pragma once

#include <imgui.h>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace ImWidgetV4::SnapshotInternal {

struct FRegisteredTextureData {
    std::vector<std::uint8_t> Pixels;
    int Width = 0;
    int Height = 0;
    int BytesPerPixel = 4;
};

void RegisterTexture(ImTextureID textureId, const FRegisteredTextureData& textureData);
void UnregisterTexture(ImTextureID textureId);
const FRegisteredTextureData* FindTexture(ImTextureID textureId);

} // namespace ImWidgetV4::SnapshotInternal
