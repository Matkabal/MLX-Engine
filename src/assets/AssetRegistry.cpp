#include "assets/AssetRegistry.h"
#include "core/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace assets
{
    bool AssetRegistry::ScanGltf(const std::string& rootDirectory, std::string* outError)
    {
        LOG_METHOD("AssetRegistry", "ScanGltf");
        assetPaths_.clear();

        namespace fs = std::filesystem;
        const fs::path root(rootDirectory);
        if (!fs::exists(root) || !fs::is_directory(root))
        {
            if (outError)
            {
                *outError = "Asset root not found: " + rootDirectory;
            }
            return false;
        }

        for (const auto& entry : fs::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::string ext = entry.path().extension().string();
            std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (ext == ".gltf" || ext == ".glb")
            {
                assetPaths_.push_back(entry.path().string());
            }
        }

        std::sort(assetPaths_.begin(), assetPaths_.end());
        if (assetPaths_.empty())
        {
            if (outError)
            {
                *outError = "No .gltf/.glb files found in: " + rootDirectory;
            }
            return false;
        }

        return true;
    }
}
