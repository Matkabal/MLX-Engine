#pragma once

#include "math/mat4.h"

namespace math
{
    struct Transform
    {
        Vec3 position{0.0f, 0.0f, 0.0f};
        Vec3 rotationRadians{0.0f, 0.0f, 0.0f};
        Vec3 scale{1.0f, 1.0f, 1.0f};

        Mat4 ToMatrix() const
        {
            return TRS(position, rotationRadians, scale);
        }
    };
}
