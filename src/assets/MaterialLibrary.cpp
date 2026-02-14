#include "assets/MaterialLibrary.h"
#include "core/Logger.h"

#include <cmath>
#include <filesystem>
#include <fstream>

#include "json.hpp"

namespace assets
{
    namespace
    {
        constexpr float kDegToRad = 0.01745329251994329577f;
    }

    bool MaterialLibrary::LoadFromFile(const std::string& filePath, std::string* outError)
    {
        LOG_METHOD("MaterialLibrary", "LoadFromFile");
        shaders_.clear();
        assetToShader_.clear();
        renderConfigByAsset_.clear();
        defaultShaderId_ = "default_color";

        std::ifstream input(filePath);
        if (!input.is_open())
        {
            LoadDefault();
            LOG_WARN("MaterialLibrary", "LoadFromFile", "materials.json not found, using defaults.");
            if (outError)
            {
                *outError = "materials.json not found. Using default shader mapping.";
            }
            return true;
        }

        nlohmann::json json;
        try
        {
            input >> json;
        }
        catch (const std::exception& ex)
        {
            LoadDefault();
            LOG_ERROR("MaterialLibrary", "LoadFromFile", ex.what());
            if (outError)
            {
                *outError = std::string("Invalid materials.json. Using defaults. Reason: ") + ex.what();
            }
            return true;
        }

        if (json.contains("defaultShaderId") && json["defaultShaderId"].is_string())
        {
            defaultShaderId_ = json["defaultShaderId"].get<std::string>();
        }

        if (json.contains("shaders") && json["shaders"].is_array())
        {
            for (const auto& item : json["shaders"])
            {
                if (!item.contains("id") || !item.contains("vs") || !item.contains("ps"))
                {
                    continue;
                }

                ShaderDefinition def{};
                def.id = item["id"].get<std::string>();
                def.vertexShaderPath = item["vs"].get<std::string>();
                def.pixelShaderPath = item["ps"].get<std::string>();
                shaders_[def.id] = def;
            }
        }

        if (json.contains("assetBindings") && json["assetBindings"].is_array())
        {
            for (const auto& item : json["assetBindings"])
            {
                if (!item.contains("asset") || !item.contains("shaderId"))
                {
                    continue;
                }
                const std::string assetName = item["asset"].get<std::string>();
                const std::string shaderId = item["shaderId"].get<std::string>();
                assetToShader_[assetName] = shaderId;

                AssetRenderConfig cfg{};
                cfg.shaderId = shaderId;

                if (item.contains("objects") && item["objects"].is_array())
                {
                    for (const auto& obj : item["objects"])
                    {
                        ObjectSpawnConfig o{};
                        if (obj.contains("position") && obj["position"].is_array() && obj["position"].size() >= 3)
                        {
                            o.transform.position.x = obj["position"][0].get<float>();
                            o.transform.position.y = obj["position"][1].get<float>();
                            o.transform.position.z = obj["position"][2].get<float>();
                        }
                        else if (obj.contains("offset") && obj["offset"].is_array() && obj["offset"].size() >= 3)
                        {
                            // Backward compatibility for older materials.json schema.
                            o.transform.position.x = obj["offset"][0].get<float>();
                            o.transform.position.y = obj["offset"][1].get<float>();
                            o.transform.position.z = obj["offset"][2].get<float>();
                        }

                        if (obj.contains("rotation") && obj["rotation"].is_array() && obj["rotation"].size() >= 3)
                        {
                            o.transform.rotationRadians.x = obj["rotation"][0].get<float>();
                            o.transform.rotationRadians.y = obj["rotation"][1].get<float>();
                            o.transform.rotationRadians.z = obj["rotation"][2].get<float>();
                        }
                        else if (obj.contains("rotationDeg") && obj["rotationDeg"].is_array() && obj["rotationDeg"].size() >= 3)
                        {
                            o.transform.rotationRadians.x = obj["rotationDeg"][0].get<float>() * kDegToRad;
                            o.transform.rotationRadians.y = obj["rotationDeg"][1].get<float>() * kDegToRad;
                            o.transform.rotationRadians.z = obj["rotationDeg"][2].get<float>() * kDegToRad;
                        }

                        if (obj.contains("scale") && obj["scale"].is_array() && obj["scale"].size() >= 3)
                        {
                            o.transform.scale.x = obj["scale"][0].get<float>();
                            o.transform.scale.y = obj["scale"][1].get<float>();
                            o.transform.scale.z = obj["scale"][2].get<float>();
                        }

                        if (obj.contains("motion") && obj["motion"].is_object())
                        {
                            const auto& m = obj["motion"];
                            o.motion.enabled = m.value("enabled", false);
                            o.motion.amplitude = m.value("amplitude", 0.0f);
                            o.motion.speed = m.value("speed", 1.0f);
                        }

                        if (obj.contains("physics") && obj["physics"].is_object())
                        {
                            const auto& p = obj["physics"];
                            o.physics.enabled = p.value("enabled", false);
                            o.physics.stiffness = p.value("stiffness", 10.0f);
                            o.physics.damping = p.value("damping", 4.0f);
                        }

                        cfg.objects.push_back(o);
                    }
                }

                if (cfg.objects.empty())
                {
                    cfg.objects.push_back(ObjectSpawnConfig{});
                }

                renderConfigByAsset_[assetName] = cfg;
            }
        }

        if (shaders_.empty())
        {
            LoadDefault();
        }
        else if (shaders_.find(defaultShaderId_) == shaders_.end())
        {
            defaultShaderId_ = shaders_.begin()->first;
        }

        return true;
    }

    std::string MaterialLibrary::ResolveShaderIdForAsset(const std::string& assetPath) const
    {
        LOG_METHOD("MaterialLibrary", "ResolveShaderIdForAsset");
        const std::string fileName = std::filesystem::path(assetPath).filename().string();

        const auto it = assetToShader_.find(fileName);
        if (it != assetToShader_.end())
        {
            return it->second;
        }

        return defaultShaderId_;
    }

    AssetRenderConfig MaterialLibrary::ResolveRenderConfigForAsset(const std::string& assetPath) const
    {
        LOG_METHOD("MaterialLibrary", "ResolveRenderConfigForAsset");
        const std::string fileName = std::filesystem::path(assetPath).filename().string();
        const auto it = renderConfigByAsset_.find(fileName);
        if (it != renderConfigByAsset_.end())
        {
            return it->second;
        }

        AssetRenderConfig cfg{};
        cfg.shaderId = defaultShaderId_;
        cfg.objects.push_back(ObjectSpawnConfig{});
        return cfg;
    }

    bool MaterialLibrary::TryGetShaderDefinition(const std::string& shaderId, ShaderDefinition& outDefinition) const
    {
        LOG_METHOD("MaterialLibrary", "TryGetShaderDefinition");
        const auto it = shaders_.find(shaderId);
        if (it == shaders_.end())
        {
            return false;
        }
        outDefinition = it->second;
        return true;
    }

    void MaterialLibrary::LoadDefault()
    {
        LOG_METHOD("MaterialLibrary", "LoadDefault");
        ShaderDefinition def{};
        def.id = "default_color";
        def.vertexShaderPath = "shaders/triangle_vs.hlsl";
        def.pixelShaderPath = "shaders/triangle_ps.hlsl";
        shaders_[def.id] = def;
        defaultShaderId_ = def.id;
        renderConfigByAsset_.clear();
    }
}
