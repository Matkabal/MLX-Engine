#include "assets/AssetManager.h"
#include "core/Logger.h"

namespace assets
{
    std::shared_ptr<const LoadedGltfAsset> AssetManager::LoadGltf(const std::string& assetPath, std::string* outError)
    {
        LOG_METHOD("AssetManager", "LoadGltf");
        const auto it = gltfCache_.find(assetPath);
        if (it != gltfCache_.end())
        {
            return it->second;
        }

        auto loaded = std::make_shared<LoadedGltfAsset>();
        loaded->sourcePath = assetPath;

        std::string loadError;
        if (!gltfLoader_.LoadFromFile(assetPath, loaded->scene, &loadError))
        {
            if (outError)
            {
                *outError = loadError;
            }
            return nullptr;
        }

        gltfCache_[assetPath] = loaded;
        return loaded;
    }
}
