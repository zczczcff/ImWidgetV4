#include "TextureRegistry.h"

namespace ImWidgetV4::SnapshotInternal {
namespace {

std::unordered_map<ImTextureID, FRegisteredTextureData>& GetTextureRegistry()
{
    static std::unordered_map<ImTextureID, FRegisteredTextureData> registry;
    return registry;
}

} // namespace

void RegisterTexture(ImTextureID textureId, const FRegisteredTextureData& textureData)
{
    if (textureId == nullptr || textureData.Width <= 0 || textureData.Height <= 0 || textureData.Pixels.empty()) {
        return;
    }

    GetTextureRegistry()[textureId] = textureData;
}

void UnregisterTexture(ImTextureID textureId)
{
    if (textureId == nullptr) {
        return;
    }

    GetTextureRegistry().erase(textureId);
}

const FRegisteredTextureData* FindTexture(ImTextureID textureId)
{
    const auto& registry = GetTextureRegistry();
    const auto it = registry.find(textureId);
    if (it == registry.end()) {
        return nullptr;
    }

    return &it->second;
}

} // namespace ImWidgetV4::SnapshotInternal
