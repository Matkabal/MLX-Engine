#pragma once

#include <cstdint>
#include <vector>

#include "geometry/MeshTypes.h"

namespace geometry
{
    struct MeshData
    {
        std::vector<ColoredVertex> vertices;
        std::vector<uint32_t> indices;
    };

    class TowelMeshGenerator
    {
    public:
        static MeshData Generate(
            uint32_t columns,
            uint32_t rows,
            float width,
            float height,
            float waveAmplitude,
            float waveFrequency);
    };
}
