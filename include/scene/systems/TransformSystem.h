#pragma once

#include "ecs/ComponentSystem.h"
#include "scene/components/TransformComponent.h"

namespace scene
{
    class TransformSystem
    {
    public:
        void Update(ecs::ComponentSystem& components) const;
    };
}
