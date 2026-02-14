#pragma once

#include "editor/ImGuiSupport.h"
#include "editor/SceneCamera.h"

namespace editor
{
    struct SceneViewFrameData
    {
        float viewportWidth = 1.0f;
        float viewportHeight = 1.0f;
        float ndcX = 0.0f;
        float ndcY = 0.0f;
        SceneCameraInput cameraInput{};
        bool hovered = false;
    };

    class SceneView
    {
    public:
        SceneViewFrameData Draw();
    };
}

