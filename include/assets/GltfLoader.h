#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "math/transform.h"
#include "math/vec2.h"
#include "math/vec3.h"
#include "math/vec4.h"

namespace assets
{
    struct MeshVertex
    {
        math::Vec3 position{};
        math::Vec3 normal{};
        math::Vec2 uv{};
    };

    struct MeshPrimitive
    {
        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        int materialIndex = -1;
    };

    struct GltfMeshData
    {
        std::string name;
        std::vector<MeshPrimitive> primitives;
    };

    struct GltfMaterialData
    {
        math::Vec4 baseColorFactor{1.0f, 1.0f, 1.0f, 1.0f};
        std::string baseColorTexturePath;
        std::string normalTexturePath;
    };

    struct GltfNodeData
    {
        std::string name;
        math::Transform localTransform{};
        int meshIndex = -1;
        int parentIndex = -1;
        std::vector<int> children;
    };

    struct GltfSceneData
    {
        std::vector<GltfMeshData> meshes;
        std::vector<GltfMaterialData> materials;
        std::vector<GltfNodeData> nodes;
        std::vector<int> rootNodes;
    };

    class GltfLoader
    {
    public:
        // Loads .gltf or .glb and extracts mesh primitives in triangle topology.
        // Didactic: material and skinning are intentionally deferred for next modules.
        bool LoadFromFile(const std::string& filePath, GltfSceneData& outScene, std::string* outError = nullptr) const;
    };
}
