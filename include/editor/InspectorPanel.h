#pragma once

#include "ecs/Entity.h"
#include "editor/ImGuiSupport.h"
#include "scene/Scene.h"

namespace editor
{
    class InspectorPanel
    {
    public:
        void Draw(scene::Scene& scene, ecs::Entity selectedEntity);
    };
}

