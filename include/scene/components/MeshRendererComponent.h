#pragma once

#include <string>

namespace scene
{
    struct MeshRendererComponent
    {
        std::string assetPath;
        int meshIndex = -1;
        int primitiveIndex = -1;
        bool visible = true;
    };
}
