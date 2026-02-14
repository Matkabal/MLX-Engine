#include "assets/SceneRepository.h"

#include <filesystem>
#include <fstream>

#include "core/Logger.h"
#include "json.hpp"

namespace assets
{
    namespace
    {
        nlohmann::json ObjectToJson(const SceneObjectSpec& obj)
        {
            return {
                {"asset", obj.assetPath},
                {"meshIndex", obj.meshIndex},
                {"primitiveIndex", obj.primitiveIndex},
                {"position", nlohmann::json::array({obj.transform.position.x, obj.transform.position.y, obj.transform.position.z})},
                {"rotation", nlohmann::json::array({obj.transform.rotationRadians.x, obj.transform.rotationRadians.y, obj.transform.rotationRadians.z})},
                {"scale", nlohmann::json::array({obj.transform.scale.x, obj.transform.scale.y, obj.transform.scale.z})},
            };
        }

        SceneObjectSpec JsonToObject(const nlohmann::json& j)
        {
            SceneObjectSpec obj{};
            obj.assetPath = j.value("asset", "");
            obj.meshIndex = j.value("meshIndex", -1);
            obj.primitiveIndex = j.value("primitiveIndex", -1);
            if (j.contains("position") && j["position"].is_array() && j["position"].size() >= 3)
            {
                obj.transform.position.x = j["position"][0].get<float>();
                obj.transform.position.y = j["position"][1].get<float>();
                obj.transform.position.z = j["position"][2].get<float>();
            }
            if (j.contains("rotation") && j["rotation"].is_array() && j["rotation"].size() >= 3)
            {
                obj.transform.rotationRadians.x = j["rotation"][0].get<float>();
                obj.transform.rotationRadians.y = j["rotation"][1].get<float>();
                obj.transform.rotationRadians.z = j["rotation"][2].get<float>();
            }
            if (j.contains("scale") && j["scale"].is_array() && j["scale"].size() >= 3)
            {
                obj.transform.scale.x = j["scale"][0].get<float>();
                obj.transform.scale.y = j["scale"][1].get<float>();
                obj.transform.scale.z = j["scale"][2].get<float>();
            }
            return obj;
        }
    } // namespace

    bool SceneRepository::EnsureDefaultScene(const std::string& projectPath, std::string& outScenePath, std::string* outError)
    {
        LOG_METHOD("SceneRepository", "EnsureDefaultScene");
        namespace fs = std::filesystem;
        const fs::path scenesDir = fs::path(projectPath) / "scenes";
        std::error_code ec;
        fs::create_directories(scenesDir, ec);
        if (ec)
        {
            if (outError)
            {
                *outError = "Failed creating scenes directory.";
            }
            return false;
        }

        const fs::path defaultScene = scenesDir / "default.scene.json";
        outScenePath = defaultScene.string();
        if (fs::exists(defaultScene))
        {
            return true;
        }

        std::vector<SceneObjectSpec> initial{
            SceneObjectSpec{"triangle.gltf", -1, -1, math::Transform{}},
        };
        return SaveScene(defaultScene.string(), initial, outError);
    }

    bool SceneRepository::LoadScene(const std::string& scenePath, std::vector<SceneObjectSpec>& outObjects, std::string* outError)
    {
        LOG_METHOD("SceneRepository", "LoadScene");
        outObjects.clear();
        nlohmann::json j;
        std::ifstream in(scenePath);
        if (!in.is_open())
        {
            if (outError)
            {
                *outError = "Scene file not found: " + scenePath;
            }
            return false;
        }

        try
        {
            in >> j;
        }
        catch (const std::exception& ex)
        {
            if (outError)
            {
                *outError = ex.what();
            }
            return false;
        }

        if (j.contains("objects") && j["objects"].is_array())
        {
            for (const auto& item : j["objects"])
            {
                SceneObjectSpec obj = JsonToObject(item);
                if (!obj.assetPath.empty())
                {
                    outObjects.push_back(obj);
                }
            }
        }
        return true;
    }

    bool SceneRepository::SaveScene(const std::string& scenePath, const std::vector<SceneObjectSpec>& objects, std::string* outError)
    {
        LOG_METHOD("SceneRepository", "SaveScene");
        nlohmann::json j;
        j["objects"] = nlohmann::json::array();
        for (const SceneObjectSpec& obj : objects)
        {
            j["objects"].push_back(ObjectToJson(obj));
        }

        std::error_code ec;
        std::filesystem::create_directories(std::filesystem::path(scenePath).parent_path(), ec);
        std::ofstream out(scenePath);
        if (!out.is_open())
        {
            if (outError)
            {
                *outError = "Failed to save scene file.";
            }
            return false;
        }
        out << j.dump(2);
        return true;
    }
}
