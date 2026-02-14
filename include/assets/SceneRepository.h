#pragma once

#include <string>
#include <vector>

#include "math/transform.h"

namespace assets
{
    struct SceneObjectSpec
    {
        std::string assetPath;
        int meshIndex = -1;
        int primitiveIndex = -1;
        math::Transform transform{};
    };

    class SceneRepository
    {
    public:
        static bool EnsureDefaultScene(const std::string& projectPath, std::string& outScenePath, std::string* outError = nullptr);
        static bool LoadScene(const std::string& scenePath, std::vector<SceneObjectSpec>& outObjects, std::string* outError = nullptr);
        static bool SaveScene(const std::string& scenePath, const std::vector<SceneObjectSpec>& objects, std::string* outError = nullptr);
    };
}
