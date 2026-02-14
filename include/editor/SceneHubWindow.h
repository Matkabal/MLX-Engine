#pragma once

#include <string>

#include <windows.h>

namespace editor
{
    class SceneHubWindow
    {
    public:
        static bool ShowModal(
            HINSTANCE instance,
            const std::string& projectPath,
            const std::string& modelsDirectory,
            std::string& outScenePath,
            std::string& outPlacementAsset);
    };
}
