#pragma once

#include <string>
#include <vector>

#include "assets/AssetTypes.h"

namespace assets
{
    class AssetCatalog
    {
    public:
        bool LoadAllGltfFromDirectory(const std::string& directoryPath, std::string* outError = nullptr);

        const std::vector<LoadedGltfAsset>& GetLoadedGltfAssets() const { return gltfAssets_; }
        size_t GetLoadedGltfCount() const { return gltfAssets_.size(); }

    private:
        GltfLoader gltfLoader_{};
        std::vector<LoadedGltfAsset> gltfAssets_{};
    };
}
