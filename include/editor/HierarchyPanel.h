#pragma once

#include "ecs/Entity.h"
#include "editor/ImGuiSupport.h"
#include "scene/Scene.h"

namespace editor
{
    class HierarchyPanel
    {
    public:
        void Draw(scene::Scene& scene, ecs::Entity& selectedEntity);
    };
}

