#include "geometry/TowelMeshGenerator.h"
#include "core/Logger.h"

#include <algorithm>
#include <cmath>

namespace geometry
{
    MeshData TowelMeshGenerator::Generate(
        uint32_t columns,
        uint32_t rows,
        float width,
        float height,
        float waveAmplitude,
        float waveFrequency)
    {
        LOG_METHOD("TowelMeshGenerator", "Generate");
        MeshData mesh{};

        columns = std::max<uint32_t>(columns, 2);
        rows = std::max<uint32_t>(rows, 2);

        const uint32_t vertexCount = columns * rows;
        mesh.vertices.reserve(vertexCount);

        for (uint32_t y = 0; y < rows; ++y)
        {
            for (uint32_t x = 0; x < columns; ++x)
            {
                const float u = static_cast<float>(x) / static_cast<float>(columns - 1);
                const float v = static_cast<float>(y) / static_cast<float>(rows - 1);

                const float px = (u - 0.5f) * width;
                const float pz = (v - 0.5f) * height;

                // Simple cloth-like folds: two sine waves + slight corner sag.
                const float foldA = std::sin((u * waveFrequency) * 6.2831853f);
                const float foldB = std::cos((v * waveFrequency * 0.7f) * 6.2831853f);
                const float cornerSag = (u * v) * 0.35f;
                const float py = (foldA * 0.6f + foldB * 0.4f - cornerSag) * waveAmplitude;

                // Towel-like checker tint.
                const int checker = (static_cast<int>(u * 8.0f) + static_cast<int>(v * 8.0f)) & 1;
                const float base = checker == 0 ? 0.82f : 0.72f;
                const float r = base;
                const float g = base * 0.88f;
                const float b = base * 0.82f;

                mesh.vertices.push_back({{px, py, pz}, {r, g, b}});
            }
        }

        const uint32_t quadCount = (columns - 1) * (rows - 1);
        mesh.indices.reserve(quadCount * 6);

        for (uint32_t y = 0; y < rows - 1; ++y)
        {
            for (uint32_t x = 0; x < columns - 1; ++x)
            {
                const uint32_t i0 = (y * columns) + x;
                const uint32_t i1 = i0 + 1;
                const uint32_t i2 = i0 + columns;
                const uint32_t i3 = i2 + 1;

                mesh.indices.push_back(i0);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i1);

                mesh.indices.push_back(i1);
                mesh.indices.push_back(i2);
                mesh.indices.push_back(i3);
            }
        }

        return mesh;
    }
}
