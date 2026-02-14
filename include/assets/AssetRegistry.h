#pragma once

#include <string>
#include <vector>

namespace assets
{
    class AssetRegistry
    {
    public:
        bool ScanGltf(const std::string& rootDirectory, std::string* outError = nullptr);
        const std::vector<std::string>& GetAssetPaths() const { return assetPaths_; }

    private:
        std::vector<std::string> assetPaths_{};
    };
}
