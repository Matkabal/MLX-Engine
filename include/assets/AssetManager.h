#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "assets/AssetTypes.h"
#include "assets/GltfLoader.h"

namespace assets
{
    class AssetManager
    {
    public:
        std::shared_ptr<const LoadedGltfAsset> LoadGltf(const std::string& assetPath, std::string* outError = nullptr);

    private:
        GltfLoader gltfLoader_{};
        std::unordered_map<std::string, std::shared_ptr<LoadedGltfAsset>> gltfCache_{};
    };
}
