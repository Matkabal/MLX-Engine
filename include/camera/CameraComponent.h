#pragma once

#include "math/mat4.h"
#include "math/vec3.h"

namespace camera
{
    struct CameraComponent
    {
        math::Vec3 target{0.0f, 0.0f, 0.0f};
        float distance = 4.5f;
        float yaw = 0.0f;
        float pitch = 0.35f;

        math::Vec3 position{0.0f, 0.0f, -4.5f};
        math::Mat4 viewMatrix = math::Identity();
        math::Mat4 projectionMatrix = math::Identity();

        float zoomSpeed = 3.5f;
        float panSpeed = 2.4f;
        float orbitSpeed = 1.9f;
        float minDistance = 0.2f;
        float maxDistance = 300.0f;
        float fovRadians = 1.04719755f; // 60 deg
        float nearPlane = 0.05f;
        float farPlane = 1000.0f;
    };
}

