#pragma once

#include "ecs/Entity.h"
#include "math/mat4.h"
#include "math/transform.h"

namespace scene
{
    struct TransformComponent
    {
        math::Transform local{};
        ecs::Entity parent = ecs::kInvalidEntity;
        math::Mat4 world = math::Identity();
    };
}
