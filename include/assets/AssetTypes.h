#pragma once

#include <string>

#include "assets/GltfLoader.h"

namespace assets
{
    struct LoadedGltfAsset
    {
        std::string sourcePath;
        GltfSceneData scene;
    };
}
