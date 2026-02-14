#pragma once

#include <string>

namespace scene
{
    struct MaterialComponent
    {
        // Logical material id resolved by MaterialLibrary.
        std::string materialId = "default_color";
    };
}
