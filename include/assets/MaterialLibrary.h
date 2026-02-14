#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "math/transform.h"

namespace assets
{
    struct ShaderDefinition
    {
        std::string id;
        std::string vertexShaderPath;
        std::string pixelShaderPath;
    };

    struct MotionConfig
    {
        bool enabled = false;
        float amplitude = 0.0f;
        float speed = 1.0f;
    };

    struct PhysicsConfig
    {
        bool enabled = false;
        float stiffness = 10.0f;
        float damping = 4.0f;
    };

    struct ObjectSpawnConfig
    {
        math::Transform transform{};
        MotionConfig motion{};
        PhysicsConfig physics{};
    };

    struct AssetRenderConfig
    {
        std::string shaderId;
        std::vector<ObjectSpawnConfig> objects;
    };

    class MaterialLibrary
    {
    public:
        bool LoadFromFile(const std::string& filePath, std::string* outError = nullptr);
        std::string ResolveShaderIdForAsset(const std::string& assetPath) const;
        AssetRenderConfig ResolveRenderConfigForAsset(const std::string& assetPath) const;
        bool TryGetShaderDefinition(const std::string& shaderId, ShaderDefinition& outDefinition) const;

    private:
        void LoadDefault();

    private:
        std::unordered_map<std::string, ShaderDefinition> shaders_{};
        std::unordered_map<std::string, std::string> assetToShader_{};
        std::unordered_map<std::string, AssetRenderConfig> renderConfigByAsset_{};
        std::string defaultShaderId_ = "default_color";
    };
}
