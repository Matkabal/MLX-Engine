#include "assets/AssetCatalog.h"
#include "core/Logger.h"

#include <algorithm>
#include <cctype>
#include <filesystem>

namespace assets
{
    bool AssetCatalog::LoadAllGltfFromDirectory(const std::string& directoryPath, std::string* outError)
    {
        LOG_METHOD("AssetCatalog", "LoadAllGltfFromDirectory");
        gltfAssets_.clear();

        namespace fs = std::filesystem;
        const fs::path root(directoryPath);
        if (!fs::exists(root) || !fs::is_directory(root))
        {
            if (outError)
            {
                *outError = "Asset directory not found: " + directoryPath;
            }
            return false;
        }

        std::vector<fs::path> filesToLoad;
        for (const auto& entry : fs::recursive_directory_iterator(root))
        {
            if (!entry.is_regular_file())
            {
                continue;
            }

            std::string extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(), [](unsigned char c) {
                return static_cast<char>(std::tolower(c));
            });

            if (extension == ".gltf" || extension == ".glb")
            {
                filesToLoad.push_back(entry.path());
            }
        }

        std::sort(filesToLoad.begin(), filesToLoad.end());

        if (filesToLoad.empty())
        {
            if (outError)
            {
                *outError = "No .gltf or .glb files found in: " + directoryPath;
            }
            return false;
        }

        std::string errors;
        for (const auto& assetPath : filesToLoad)
        {
            LoadedGltfAsset loaded{};
            loaded.sourcePath = assetPath.string();

            std::string loadError;
            if (!gltfLoader_.LoadFromFile(loaded.sourcePath, loaded.scene, &loadError))
            {
                errors += "Failed: " + loaded.sourcePath + "\n" + loadError + "\n";
                continue;
            }

            gltfAssets_.push_back(std::move(loaded));
        }

        if (gltfAssets_.empty())
        {
            if (outError)
            {
                *outError = errors.empty() ? "No glTF assets were loaded." : errors;
            }
            return false;
        }

        if (outError && !errors.empty())
        {
            *outError = errors;
        }

        return true;
    }
}
