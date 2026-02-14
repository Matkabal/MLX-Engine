#include "assets/GltfLoader.h"

#include "core/Logger.h"

#include <cmath>
#include <cstring>
#include <filesystem>
#include <utility>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

namespace assets
{
    namespace
    {
        bool GetAccessorView(
            const tinygltf::Model& model,
            const tinygltf::Accessor& accessor,
            const tinygltf::BufferView*& outView,
            const unsigned char*& outData)
        {
            if (accessor.bufferView < 0 || accessor.bufferView >= static_cast<int>(model.bufferViews.size()))
            {
                return false;
            }

            const tinygltf::BufferView& view = model.bufferViews[accessor.bufferView];
            if (view.buffer < 0 || view.buffer >= static_cast<int>(model.buffers.size()))
            {
                return false;
            }

            const tinygltf::Buffer& buffer = model.buffers[view.buffer];
            const size_t byteOffset = static_cast<size_t>(view.byteOffset + accessor.byteOffset);
            if (byteOffset >= buffer.data.size())
            {
                return false;
            }

            outView = &view;
            outData = buffer.data.data() + byteOffset;
            return true;
        }

        bool ReadVec3Accessor(const tinygltf::Model& model, int accessorIndex, std::vector<math::Vec3>& outValues)
        {
            if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
            {
                return false;
            }

            const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
            if (accessor.type != TINYGLTF_TYPE_VEC3 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                return false;
            }

            const tinygltf::BufferView* view = nullptr;
            const unsigned char* data = nullptr;
            if (!GetAccessorView(model, accessor, view, data))
            {
                return false;
            }

            const size_t stride = accessor.ByteStride(*view) == 0 ? sizeof(float) * 3 : accessor.ByteStride(*view);
            outValues.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const float* src = reinterpret_cast<const float*>(data + (i * stride));
                outValues[i] = math::Vec3{src[0], src[1], src[2]};
            }
            return true;
        }

        bool ReadVec2Accessor(const tinygltf::Model& model, int accessorIndex, std::vector<math::Vec2>& outValues)
        {
            if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
            {
                return false;
            }

            const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
            if (accessor.type != TINYGLTF_TYPE_VEC2 || accessor.componentType != TINYGLTF_COMPONENT_TYPE_FLOAT)
            {
                return false;
            }

            const tinygltf::BufferView* view = nullptr;
            const unsigned char* data = nullptr;
            if (!GetAccessorView(model, accessor, view, data))
            {
                return false;
            }

            const size_t stride = accessor.ByteStride(*view) == 0 ? sizeof(float) * 2 : accessor.ByteStride(*view);
            outValues.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const float* src = reinterpret_cast<const float*>(data + (i * stride));
                outValues[i] = math::Vec2{src[0], src[1]};
            }
            return true;
        }

        bool ReadIndices(const tinygltf::Model& model, int accessorIndex, std::vector<uint32_t>& outIndices)
        {
            if (accessorIndex < 0 || accessorIndex >= static_cast<int>(model.accessors.size()))
            {
                return false;
            }

            const tinygltf::Accessor& accessor = model.accessors[accessorIndex];
            const tinygltf::BufferView* view = nullptr;
            const unsigned char* data = nullptr;
            if (!GetAccessorView(model, accessor, view, data))
            {
                return false;
            }

            size_t elementSize = 0;
            switch (accessor.componentType)
            {
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                elementSize = sizeof(uint8_t);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                elementSize = sizeof(uint16_t);
                break;
            case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                elementSize = sizeof(uint32_t);
                break;
            default:
                return false;
            }

            const size_t stride = accessor.ByteStride(*view) == 0 ? elementSize : accessor.ByteStride(*view);
            outIndices.resize(accessor.count);
            for (size_t i = 0; i < accessor.count; ++i)
            {
                const unsigned char* src = data + (i * stride);
                switch (accessor.componentType)
                {
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
                    outIndices[i] = static_cast<uint32_t>(*reinterpret_cast<const uint8_t*>(src));
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
                    outIndices[i] = static_cast<uint32_t>(*reinterpret_cast<const uint16_t*>(src));
                    break;
                case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
                    outIndices[i] = *reinterpret_cast<const uint32_t*>(src);
                    break;
                default:
                    return false;
                }
            }
            return true;
        }

        math::Vec3 QuaternionToEulerRadians(float x, float y, float z, float w)
        {
            const float sinr_cosp = 2.0f * ((w * x) + (y * z));
            const float cosr_cosp = 1.0f - (2.0f * ((x * x) + (y * y)));
            const float roll = std::atan2(sinr_cosp, cosr_cosp);

            const float sinp = 2.0f * ((w * y) - (z * x));
            const float pitch = (std::fabs(sinp) >= 1.0f) ? std::copysign(1.57079632679f, sinp) : std::asin(sinp);

            const float siny_cosp = 2.0f * ((w * z) + (x * y));
            const float cosy_cosp = 1.0f - (2.0f * ((y * y) + (z * z)));
            const float yaw = std::atan2(siny_cosp, cosy_cosp);

            return math::Vec3{roll, pitch, yaw};
        }
    } // namespace

    bool GltfLoader::LoadFromFile(const std::string& filePath, GltfSceneData& outScene, std::string* outError) const
    {
        LOG_METHOD("GltfLoader", "LoadFromFile");
        outScene.meshes.clear();
        outScene.materials.clear();
        outScene.nodes.clear();
        outScene.rootNodes.clear();

        tinygltf::TinyGLTF loader;
        tinygltf::Model model;
        std::string warn;
        std::string err;

        const bool isGlb = filePath.size() >= 4 && filePath.substr(filePath.size() - 4) == ".glb";
        const bool loaded = isGlb
            ? loader.LoadBinaryFromFile(&model, &err, &warn, filePath)
            : loader.LoadASCIIFromFile(&model, &err, &warn, filePath);

        if (!warn.empty() && outError)
        {
            *outError += "Warning: " + warn + "\n";
        }
        if (!loaded)
        {
            if (outError)
            {
                *outError += "Failed to load glTF: " + err;
            }
            return false;
        }

        const std::filesystem::path baseDir = std::filesystem::path(filePath).parent_path();

        outScene.materials.reserve(model.materials.size());
        for (const auto& mat : model.materials)
        {
            GltfMaterialData m{};
            const auto& pbr = mat.pbrMetallicRoughness;
            if (pbr.baseColorFactor.size() == 4)
            {
                m.baseColorFactor = math::Vec4{
                    static_cast<float>(pbr.baseColorFactor[0]),
                    static_cast<float>(pbr.baseColorFactor[1]),
                    static_cast<float>(pbr.baseColorFactor[2]),
                    static_cast<float>(pbr.baseColorFactor[3]),
                };
            }
            if (pbr.baseColorTexture.index >= 0 && pbr.baseColorTexture.index < static_cast<int>(model.textures.size()))
            {
                const tinygltf::Texture& tex = model.textures[pbr.baseColorTexture.index];
                if (tex.source >= 0 && tex.source < static_cast<int>(model.images.size()))
                {
                    const tinygltf::Image& image = model.images[tex.source];
                    if (!image.uri.empty())
                    {
                        m.baseColorTexturePath = (baseDir / image.uri).string();
                    }
                }
            }
            outScene.materials.push_back(std::move(m));
        }

        outScene.meshes.reserve(model.meshes.size());
        for (const tinygltf::Mesh& mesh : model.meshes)
        {
            GltfMeshData meshData{};
            meshData.name = mesh.name;

            for (const tinygltf::Primitive& primitive : mesh.primitives)
            {
                if (primitive.mode != TINYGLTF_MODE_TRIANGLES)
                {
                    continue;
                }

                const auto posIt = primitive.attributes.find("POSITION");
                if (posIt == primitive.attributes.end())
                {
                    continue;
                }

                std::vector<math::Vec3> positions;
                if (!ReadVec3Accessor(model, posIt->second, positions))
                {
                    continue;
                }

                std::vector<math::Vec3> normals(positions.size(), math::Vec3{0.0f, 0.0f, 1.0f});
                const auto normalIt = primitive.attributes.find("NORMAL");
                if (normalIt != primitive.attributes.end())
                {
                    std::vector<math::Vec3> tempNormals;
                    if (ReadVec3Accessor(model, normalIt->second, tempNormals) && tempNormals.size() == positions.size())
                    {
                        normals = std::move(tempNormals);
                    }
                }

                std::vector<math::Vec2> uvs(positions.size(), math::Vec2{0.0f, 0.0f});
                const auto uvIt = primitive.attributes.find("TEXCOORD_0");
                if (uvIt != primitive.attributes.end())
                {
                    std::vector<math::Vec2> tempUvs;
                    if (ReadVec2Accessor(model, uvIt->second, tempUvs) && tempUvs.size() == positions.size())
                    {
                        uvs = std::move(tempUvs);
                    }
                }

                MeshPrimitive prim{};
                prim.materialIndex = primitive.material;
                prim.vertices.resize(positions.size());
                for (size_t i = 0; i < positions.size(); ++i)
                {
                    prim.vertices[i].position = positions[i];
                    prim.vertices[i].normal = normals[i];
                    prim.vertices[i].uv = uvs[i];
                }

                if (primitive.indices >= 0)
                {
                    if (!ReadIndices(model, primitive.indices, prim.indices))
                    {
                        continue;
                    }
                }
                else
                {
                    prim.indices.resize(prim.vertices.size());
                    for (size_t i = 0; i < prim.vertices.size(); ++i)
                    {
                        prim.indices[i] = static_cast<uint32_t>(i);
                    }
                }

                meshData.primitives.push_back(std::move(prim));
            }

            if (!meshData.primitives.empty())
            {
                outScene.meshes.push_back(std::move(meshData));
            }
        }

        outScene.nodes.resize(model.nodes.size());
        for (size_t i = 0; i < model.nodes.size(); ++i)
        {
            const tinygltf::Node& n = model.nodes[i];
            GltfNodeData node{};
            node.name = n.name;
            node.meshIndex = n.mesh;

            if (n.translation.size() == 3)
            {
                node.localTransform.position = math::Vec3{
                    static_cast<float>(n.translation[0]),
                    static_cast<float>(n.translation[1]),
                    static_cast<float>(n.translation[2])};
            }
            if (n.scale.size() == 3)
            {
                node.localTransform.scale = math::Vec3{
                    static_cast<float>(n.scale[0]),
                    static_cast<float>(n.scale[1]),
                    static_cast<float>(n.scale[2])};
            }
            if (n.rotation.size() == 4)
            {
                node.localTransform.rotationRadians = QuaternionToEulerRadians(
                    static_cast<float>(n.rotation[0]),
                    static_cast<float>(n.rotation[1]),
                    static_cast<float>(n.rotation[2]),
                    static_cast<float>(n.rotation[3]));
            }

            for (int child : n.children)
            {
                node.children.push_back(child);
            }
            outScene.nodes[i] = std::move(node);
        }

        for (size_t p = 0; p < outScene.nodes.size(); ++p)
        {
            for (int child : outScene.nodes[p].children)
            {
                if (child >= 0 && child < static_cast<int>(outScene.nodes.size()))
                {
                    outScene.nodes[static_cast<size_t>(child)].parentIndex = static_cast<int>(p);
                }
            }
        }

        if (model.defaultScene >= 0 && model.defaultScene < static_cast<int>(model.scenes.size()))
        {
            for (int root : model.scenes[model.defaultScene].nodes)
            {
                outScene.rootNodes.push_back(root);
            }
        }
        else
        {
            for (size_t i = 0; i < outScene.nodes.size(); ++i)
            {
                if (outScene.nodes[i].parentIndex < 0)
                {
                    outScene.rootNodes.push_back(static_cast<int>(i));
                }
            }
        }

        if (outScene.meshes.empty())
        {
            if (outError)
            {
                *outError += "No triangle meshes found in glTF.";
            }
            return false;
        }

        return true;
    }
}
